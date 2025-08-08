#include "orchestrator_service.h"

// ---------- Queues ----------
extern osMessageQueueId_t keypadEventQueueHandle; // legacy
extern osMessageQueueId_t orchestratorEventQueueHandle; // nouvelle queue unifiée

// Variables d'état globales (définies dans global.c)
extern volatile MachineState machine_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

// Helper pour reset le choix
static void reset_choice(void) {
    keypad_choice[0] = '\0';
}

void StartTaskOrchestrator(void *argument) {
    printf("\r\nOrchestrator Task started\r\n");
    KeypadEvent evt;
    OrchestratorEvent oevt;

    for (;;) {
        // Lecture prioritaire sur la nouvelle queue
        if (osMessageQueueGet(orchestratorEventQueueHandle, &oevt, NULL, 10) == osOK) {
            char key = 0;
            if (oevt.type == ORCH_EVT_KEYPAD) {
                key = oevt.data.key;
            } else if (oevt.type == ORCH_EVT_PAYMENT_OK) {
                key = '#'; // mapping temporaire
            } else if (oevt.type == ORCH_EVT_PAYMENT_CANCEL) {
                key = '*';
            } else {
                // autres événements à traiter
            }
            if (!key) {
                continue;
            }
            printf("[Orchestrator] Key received: %c (state=%d)\r\n", key, machine_interaction);

            switch (machine_interaction) {
                case IDLE:
                case ORDERING:
                    if (key == '*' && machine_interaction == ORDERING) {
                        printf("Annuler\r\n");
                        reset_choice();
                        machine_interaction = IDLE;
                        LcdMessage m = {0};
                        snprintf(m.line1, sizeof(m.line1), "Commande annulee !");
                        LCD_SendMessage(&m);
                    } else if (key != '*' && key != '#') {
                        size_t len = strlen((const char *)keypad_choice);
                        if (len < 2) {
                            keypad_choice[len] = key;
                            keypad_choice[len + 1] = '\0';
                            machine_interaction = ORDERING;
                            LcdMessage m = {0};
                            snprintf(m.line1, sizeof(m.line1), "Choix de boisson");
                            snprintf(m.line2, sizeof(m.line2), "%s", keypad_choice);
                            LCD_SendMessage(&m);
                        }
                        if (strlen((const char *)keypad_choice) == 2) {
                            printf("Commande à valider: %s\r\n", keypad_choice);
                            client_order = (keypad_choice[0] - '0') * 10 + (keypad_choice[1] - '0');
                            reset_choice();
                            machine_interaction = PAYING;
                            // Ici : Notifier l’ESP32, etc...
                            LcdMessage m = {0};
                            snprintf(m.line1, sizeof(m.line1), "Paiement en cours");
                            snprintf(m.line2, sizeof(m.line2), "%d", client_order);
                            LCD_SendMessage(&m);
                        }
                    }
                    break;
                case PAYING:
                    if (key == '*') {
                        printf("Paiement annulé\r\n");
                        client_order = 0;
                        machine_interaction = IDLE;
                        LcdMessage m = {0};
                        snprintf(m.line1, sizeof(m.line1), "Choisissez une");
                        snprintf(m.line2, sizeof(m.line2), "boisson");
                        LCD_SendMessage(&m);
                    } else if (key == '#') {
                        // Simulation paiement OK via '#'
                        printf("Paiement validé pour commande %d\r\n", client_order);
                        machine_interaction = DELIVERING;
                        // Mapping commande -> canal
                        uint8_t channel = MotorService_OrderToChannel(client_order);
                        if (channel == 0xFF) {
                            printf("Commande invalide: %d\r\n", client_order);
                            machine_interaction = IDLE;
                            LcdMessage err = {0};
                            snprintf(err.line1, sizeof(err.line1), "Commande invalide");
                            LCD_SendMessage(&err);
                            break;
                        }
                        // Déclenchement distribution: RUN puis STOP après délai
                        LcdMessage m1 = {0};
                        snprintf(m1.line1, sizeof(m1.line1), "Distribution");
                        snprintf(m1.line2, sizeof(m1.line2), "en cours...");
                        LCD_SendMessage(&m1);
                        MotorService_SendCommand(channel, MOTOR_CMD_RUN);
                        osDelay(500);
                        MotorService_SendCommand(channel, MOTOR_CMD_STOP);
                        client_order = 0;
                        machine_interaction = IDLE;
                        LcdMessage m2 = {0};
                        snprintf(m2.line1, sizeof(m2.line1), "Choisissez une");
                        snprintf(m2.line2, sizeof(m2.line2), "boisson");
                        LCD_SendMessage(&m2);
                    }
                    break;
                // Ajoute d'autres états au besoin (DELIVERING, etc.)
                default:
                    break;
            }
        }
    }
}