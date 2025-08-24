#include "orchestrator.h"
#include "esp_communication_service.h"
#include "watchdog_service.h"

// ---------- Queues ----------
extern osMessageQueueId_t keypadEventQueueHandle; // legacy
extern osMessageQueueId_t orchestratorEventQueueHandle; // nouvelle queue unifiée

// Variables d'état globales (définies dans global.c)
extern volatile MachineState machine_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

// Variables pour la gestion des commandes de livraison
static bool deliveryOrderInProgress = false;
static char currentDeliveryOrderId[32] = {0};
static uint8_t pendingDeliveryItems = 0;
static uint8_t completedDeliveryItems = 0;

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
    ((volatile uint8_t)client_order);
    client_order = 0;
    machine_interaction = IDLE;
    orchestrator_show(IDLE);
}

// Fonction pour gérer le début d'une commande de livraison
static void orchestrator_on_order_start(const char* order_id) {
    if (deliveryOrderInProgress) {
        printf("[ORCH] Order already in progress, ignoring new order\r\n");
        return;
    }
    
    deliveryOrderInProgress = true;
    pendingDeliveryItems = 0;
    completedDeliveryItems = 0;
    strncpy(currentDeliveryOrderId, order_id, sizeof(currentDeliveryOrderId) - 1);
    currentDeliveryOrderId[sizeof(currentDeliveryOrderId) - 1] = '\0';
    
    machine_interaction = DELIVERING;
    orchestrator_send_lcd("Commande QR recue", currentDeliveryOrderId);
    printf("[ORCH] Order started: %s\r\n", currentDeliveryOrderId);
}

// Fonction pour gérer un item de livraison
static void orchestrator_on_vend_item(uint8_t slot_number, uint8_t quantity, const char* product_id) {
    if (!deliveryOrderInProgress) {
        printf("[ORCH] VEND item received without active order\r\n");
        return;
    }
    
    pendingDeliveryItems++;
    printf("[ORCH] VEND item: slot=%d, qty=%d, product=%s\r\n", slot_number, quantity, product_id);
    
    // Démarrer la livraison pour cet item
    uint8_t channel = slot_number; // Le slot_number correspond directement au channel du multiplexeur
    if (channel >= 1 && channel <= 4) {
        // Livrer la quantité demandée
        for (uint8_t i = 0; i < quantity; i++) {
            MotorService_StartDelivery(channel);
            // Attendre un peu entre chaque livraison
            osDelay(1000);
        }
        
        // Confirmer la livraison de cet item
        char response[64];
        snprintf(response, sizeof(response), "VEND_COMPLETED:%d", slot_number);
        EspComm_SendLine(response);
        
        completedDeliveryItems++;
        printf("[ORCH] Item delivered: %d/%d\r\n", completedDeliveryItems, pendingDeliveryItems);
    } else {
        printf("[ORCH] Invalid channel: %d\r\n", channel);
        char response[64];
        snprintf(response, sizeof(response), "VEND_FAILED:%d:INVALID_CHANNEL", slot_number);
        EspComm_SendLine(response);
    }
}

// Fonction pour finaliser une commande de livraison
static void orchestrator_on_order_complete(void) {
    if (!deliveryOrderInProgress) {
        printf("[ORCH] Order complete received without active order\r\n");
        return;
    }
    
    printf("[ORCH] Order completed: %s (%d items delivered)\r\n", 
           currentDeliveryOrderId, completedDeliveryItems);
    
    // Confirmer la livraison complète à l'ESP
    EspComm_SendLine("DELIVERY_COMPLETED");
    
    // Réinitialiser l'état
    deliveryOrderInProgress = false;
    machine_interaction = IDLE;
    orchestrator_show(IDLE);
}

// Fonction pour gérer l'échec d'une commande
static void orchestrator_on_order_failed(void) {
    if (deliveryOrderInProgress) {
        printf("[ORCH] Order failed: %s\r\n", currentDeliveryOrderId);
        EspComm_SendLine("DELIVERY_FAILED:ORDER_CANCELLED");
        deliveryOrderInProgress = false;
        machine_interaction = IDLE;
        orchestrator_show(IDLE);
    }
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
        return;
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
        uint8_t ch = MotorService_OrderToChannel(orderCode);
        if (ch == 0xFF) {
            client_order = 0;
            machine_interaction = IDLE;
            orchestrator_send_lcd("Produit non ", "valable");
            osDelay(3000);
            orchestrator_show(IDLE);
            return;
        }
        client_order = orderCode;
        machine_interaction = PAYING;
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
        printf("Paiement validé pour commande %d\r\n", client_order);
        machine_interaction = DELIVERING;
        uint8_t channel = MotorService_OrderToChannel(client_order);
        if (channel == 0xFF) {
            printf("Commande invalide: %d\r\n", client_order);
            machine_interaction = IDLE;
            orchestrator_send_lcd("Commande invalide", "");
            return;
        }
        orchestrator_show(DELIVERING);
        MotorService_StartDelivery(channel);
    }
}

static void orchestrator_on_key_event(char key) {
    // Logging sécurisé sans exposition des détails internes
    LOGD("[Orchestrator] Key event processed (state=%d)\r\n", (int)machine_interaction);
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
        // Heartbeat watchdog pour signaler que la tâche est vivante
        Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
        
        if (osMessageQueueGet(orchestratorEventQueueHandle, &oevt, NULL, 1000) != osOK) {
            continue; // Timeout pour permettre le heartbeat régulier
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
                orchestrator_send_lcd("Aucune connexion", "internet");
                osDelay(3000);
                machine_interaction = IDLE;
                orchestrator_show(IDLE);
                break;
            case ORCH_EVT_DELIVERY_DONE:
                orchestrator_on_delivery_done();
                break;
            case ORCH_EVT_STOCK_LOW:
                printf("Stock LOW: sensor=%d, %dmm\r\n", oevt.data.stock.sensorId, oevt.data.stock.mm);
                break;
            case ORCH_EVT_ORDER_START:
                orchestrator_on_order_start(oevt.data.order.order_id);
                break;
            case ORCH_EVT_VEND_ITEM:
                orchestrator_on_vend_item(oevt.data.vend.slot_number, 
                                        oevt.data.vend.quantity, 
                                        oevt.data.vend.product_id);
                break;
            case ORCH_EVT_ORDER_COMPLETE:
                orchestrator_on_order_complete();
                break;
            case ORCH_EVT_ORDER_FAILED:
                orchestrator_on_order_failed();
                break;
            case ORCH_EVT_ERROR_MOTOR:
            default:
                break;
        }
    }
}


