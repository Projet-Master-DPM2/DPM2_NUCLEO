# Protocole de Livraison NUCLEO - Gestion des Commandes QR

## Vue d'ensemble

Ce document décrit les modifications apportées à la NUCLEO pour gérer les commandes de livraison envoyées par l'ESP32 via UART.

## Modifications apportées

### 1. **Types de messages ESP étendus**

Nouveaux types de messages ajoutés dans `esp_communication_service.h` :

```c
typedef enum {
    ESP_MSG_UNKNOWN = 0,
    ESP_MSG_NFC_UID,
    ESP_MSG_NFC_ERR,
    ESP_MSG_NAK_PAYING_NO_NET,
    ESP_MSG_NAK_PAYMENT_DENIED,
    ESP_MSG_ORDER_START,        // Nouveau
    ESP_MSG_VEND_COMMAND,       // Nouveau
    ESP_MSG_ORDER_END,          // Nouveau
    ESP_MSG_QR_TOKEN_ERROR,     // Nouveau
    ESP_MSG_QR_TOKEN_INVALID,   // Nouveau
    ESP_MSG_QR_TOKEN_BUSY,      // Nouveau
    ESP_MSG_QR_TOKEN_NO_NETWORK, // Nouveau
    ESP_MSG_ORDER_FAILED        // Nouveau
} EspMessageType;
```

### 2. **Types d'événements orchestrateur étendus**

Nouveaux événements ajoutés dans `orchestrator.h` :

```c
typedef enum {
    ORCH_EVT_KEYPAD,
    ORCH_EVT_PAYMENT_OK,
    ORCH_EVT_PAYMENT_CANCEL,
    ORCH_EVT_ERROR_MOTOR,
    ORCH_EVT_DELIVERY_DONE,
    ORCH_EVT_STOCK_LOW,
    ORCH_EVT_NO_NET,
    ORCH_EVT_ORDER_START,       // Nouveau
    ORCH_EVT_VEND_ITEM,         // Nouveau
    ORCH_EVT_ORDER_COMPLETE,    // Nouveau
    ORCH_EVT_ORDER_FAILED       // Nouveau
} OrchestratorEventType;
```

### 3. **Structure de données étendue**

Nouvelles structures de données pour les événements :

```c
typedef struct {
    OrchestratorEventType type;
    union {
        char key;     // for ORCH_EVT_KEYPAD
        uint8_t code; // generic small payload
        struct {
            uint8_t sensorId;
            uint8_t mm;
        } stock;      // for ORCH_EVT_STOCK_LOW
        struct {
            char order_id[32];
        } order;      // for ORCH_EVT_ORDER_START
        struct {
            uint8_t slot_number;
            uint8_t quantity;
            char product_id[32];
        } vend;       // for ORCH_EVT_VEND_ITEM
    } data;
} OrchestratorEvent;
```

## Workflow de traitement des commandes

### 1. **Réception ORDER_START**

```
ESP32 → NUCLEO: "ORDER_START:order_123456789"
```

**Traitement NUCLEO :**
- Parse l'ID de commande
- Initialise l'état de livraison
- Envoie `ORDER_ACK` à l'ESP32
- Affiche "Commande QR recue" sur le LCD
- Passe en état `DELIVERING`

### 2. **Réception des commandes VEND**

```
ESP32 → NUCLEO: "VEND 1 2 prod_coca_cola"
ESP32 → NUCLEO: "VEND 3 1 prod_sprite"
```

**Traitement NUCLEO :**
- Parse slot_number, quantity, product_id
- Valide les paramètres (slot 1-99, quantity 1-10)
- Démarre la livraison via `MotorService_StartDelivery()`
- Envoie `VEND_COMPLETED:<slot>` ou `VEND_FAILED:<slot>:<reason>`

### 3. **Réception ORDER_END**

```
ESP32 → NUCLEO: "ORDER_END"
```

**Traitement NUCLEO :**
- Finalise la commande
- Envoie `DELIVERY_COMPLETED` à l'ESP32
- Retourne à l'état `IDLE`

## Gestion des erreurs

### **Erreurs de commande**
- `QR_TOKEN_ERROR` : Erreur de validation du token
- `QR_TOKEN_INVALID` : Token invalide
- `QR_TOKEN_BUSY` : Commande en cours
- `QR_TOKEN_NO_NETWORK` : Pas de connexion réseau
- `ORDER_FAILED` : Échec de la commande

**Traitement :**
- Réinitialise l'état de commande
- Envoie `DELIVERY_FAILED:ORDER_CANCELLED`
- Retourne à l'état `IDLE`

### **Erreurs de livraison**
- `ORDER_NAK:NO_ACTIVE_ORDER` : Commande VEND sans ordre actif
- `ORDER_NAK:INVALID_VEND_FORMAT` : Format VEND invalide
- `VEND_FAILED:<slot>:INVALID_CHANNEL` : Channel invalide (doit être 1-4)

## Variables d'état ajoutées

### **Dans esp_communication_service.c**
```c
static bool orderInProgress = false;
static char currentOrderId[32] = {0};
static uint8_t totalItems = 0;
static uint8_t deliveredItems = 0;
```

### **Dans orchestrator.c**
```c
static bool deliveryOrderInProgress = false;
static char currentDeliveryOrderId[32] = {0};
static uint8_t pendingDeliveryItems = 0;
static uint8_t completedDeliveryItems = 0;
```

## Fonctions ajoutées

### **Parsing des commandes**
- `parse_vend_command()` : Parse les commandes VEND
- `parse_order_start()` : Parse les commandes ORDER_START

### **Gestion des événements**
- `orchestrator_on_order_start()` : Début de commande
- `orchestrator_on_vend_item()` : Traitement d'un item
- `orchestrator_on_order_complete()` : Finalisation
- `orchestrator_on_order_failed()` : Gestion d'échec

## Mapping slot → channel

Le `slot_number` reçu correspond directement au channel du multiplexeur (4 channels max) :
- Slot 1 → Channel 1 du multiplexeur
- Slot 2 → Channel 2 du multiplexeur
- Slot 3 → Channel 3 du multiplexeur
- Slot 4 → Channel 4 du multiplexeur

## Logging et debugging

Tous les événements sont loggés avec des préfixes :
- `[ESP_UART]` : Messages de communication UART
- `[ORCH]` : Messages de l'orchestrateur

## Intégration avec l'existant

Le système de livraison QR coexiste avec le système de commande par keypad :
- Les deux systèmes utilisent le même `MotorService`
- L'état `DELIVERING` est partagé
- Les confirmations de livraison sont unifiées

## Sécurité et validation

- Validation des paramètres (slot 1-99, quantity 1-10)
- Protection contre les commandes sans ordre actif
- Timeout et gestion d'erreurs robuste
- Logging sécurisé des événements
