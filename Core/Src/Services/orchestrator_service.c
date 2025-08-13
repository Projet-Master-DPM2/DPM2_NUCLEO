#include "orchestrator_service.h"
#include "esp_communication_service.h"

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

// ---------- Affichage LCD (helpers) ----------
static void orchestrator_send_lcd(const char* line1, const char* line2) {
    LcdMessage m = {0};
    if (line1 != NULL) {
        snprintf(m.line1, sizeof(m.line1), "%s", line1);
    }
    if (line2 != NULL) {
        snprintf(m.line2, sizeof(m.line2), "%s", line2);
    }
    LCD_SendMessage(&m);
}

static void orchestrator_show(MachineState state) {
    switch (state) {
        case IDLE:
            orchestrator_send_lcd("Choisissez une", "boisson");
            break;
        case ORDERING:
            orchestrator_send_lcd("Choix de boisson", (const char*)keypad_choice);
            break;
        case PAYING: {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", client_order);
            orchestrator_send_lcd("Paiement en cours", buf);
            break;
        }
        case DELIVERING:
            orchestrator_send_lcd("Distribution", "en cours...");
            break;
        case SETTINGS:
            orchestrator_send_lcd("Parametres", "");
            break;
        default:
            break;
    }
}

// ---------- Gestion des événements ----------
static void orchestrator_on_delivery_done(void) {
    ((volatile uint8_t)client_order); // assure l'accès mémoire mais déjà global
    client_order = 0;
    machine_interaction = IDLE;
    orchestrator_show(IDLE);
}

static void orchestrator_on_key_idle_ordering(char key) {
    if (key == '*' && machine_interaction == ORDERING) {
        printf("Annuler\r\n");
        reset_choice();
        machine_interaction = IDLE;
        orchestrator_send_lcd("Commande annulee !", "");
        return;
    }
    if (key == '*' || key == '#') {
        return; // Ignorer autres contrôles ici
    }
    size_t len = strlen((const char *)keypad_choice);
    if (len < 2) {
        keypad_choice[len] = key;
        keypad_choice[len + 1] = '\0';
        machine_interaction = ORDERING;
        orchestrator_show(ORDERING);
    }
    if (strlen((const char *)keypad_choice) == 2) {
        printf("Commande à valider: %s\r\n", keypad_choice);
        uint8_t orderCode = (keypad_choice[0] - '0') * 10 + (keypad_choice[1] - '0');
        reset_choice();
        // Valider la commande contre la table des produits autorisés
        uint8_t ch = MotorService_OrderToChannel(orderCode);
        if (ch == 0xFF) {
            // Produit non valable -> retour IDLE + message
            client_order = 0;
            machine_interaction = IDLE;
            orchestrator_send_lcd("Produit non ", "valable");
            osDelay(3000);
            orchestrator_show(IDLE);
            return;
        }
        // Produit valable -> passer à PAYING
        client_order = orderCode;
        machine_interaction = PAYING;
        // Notifier l’ESP32 (UART1)
        EspComm_SendLine("STATE:PAYING");
        orchestrator_show(PAYING);
    }
}

static void orchestrator_on_key_paying(char key) {
    if (key == '*') {
        printf("Paiement annulé\r\n");
        client_order = 0;
        machine_interaction = IDLE;
        orchestrator_show(IDLE);
        return;
    }
    if (key == '#') {
        // Simulation paiement OK via '#'
        printf("Paiement validé pour commande %d\r\n", client_order);
        machine_interaction = DELIVERING;
        // Mapping commande -> canal
        uint8_t channel = MotorService_OrderToChannel(client_order);
        if (channel == 0xFF) {
            printf("Commande invalide: %d\r\n", client_order);
            machine_interaction = IDLE;
            orchestrator_send_lcd("Commande invalide", "");
            return;
        }
        // Démarrage de la distribution asynchrone
        orchestrator_show(DELIVERING);
        MotorService_StartDelivery(channel);
    }
}

static void orchestrator_on_key_event(char key) {
    printf("[Orchestrator] Key received: %c (state=%d)\r\n", key, machine_interaction);
    switch (machine_interaction) {
        case IDLE:
        case ORDERING:
            orchestrator_on_key_idle_ordering(key);
            break;
        case PAYING:
            orchestrator_on_key_paying(key);
            break;
        default:
            break;
    }
}

void StartTaskOrchestrator(void *argument) {
    printf("\r\nOrchestrator Task started\r\n");
    OrchestratorEvent oevt;

    for (;;) {
        if (osMessageQueueGet(orchestratorEventQueueHandle, &oevt, NULL, osWaitForever) != osOK) {
            continue;
        }
        switch (oevt.type) {
            case ORCH_EVT_KEYPAD:
                orchestrator_on_key_event(oevt.data.key);
                break;
            case ORCH_EVT_PAYMENT_OK:
                orchestrator_on_key_event('#');
                break;
            case ORCH_EVT_PAYMENT_CANCEL:
                orchestrator_on_key_event('*');
                break;
            case ORCH_EVT_NO_NET:
                // Retour IDLE avec message 3s
                orchestrator_send_lcd("Aucune connexion", "internet");
                osDelay(3000);
                machine_interaction = IDLE;
                orchestrator_show(IDLE);
                break;
            case ORCH_EVT_DELIVERY_DONE:
                orchestrator_on_delivery_done();
                break;
            case ORCH_EVT_STOCK_LOW:
                // TODO: selon besoin, bloquer commande si stock bas
                printf("Stock LOW: sensor=%d, %dmm\r\n", oevt.data.stock.sensorId, oevt.data.stock.mm);
                break;
            case ORCH_EVT_ERROR_MOTOR:
            default:
                // TODO: gérer erreurs spécifiques si besoin
                break;
        }
    }
}