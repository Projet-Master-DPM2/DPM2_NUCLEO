# DPM2 NUCLEO - Module de Contrôle

## 📋 Description

Module NUCLEO-F411RE du projet **DPM (Distributeur de Produits Modulaire)** - un distributeur automatique intelligent. Le NUCLEO gère le contrôle des moteurs, la détection de stock, l'interface utilisateur et la communication avec l'ESP32.

## 🏗️ Architecture

### Rôle du NUCLEO
- **Contrôle moteur** via multiplexeur (4 canaux)
- **Détection de stock** via capteurs ToF (5 capteurs)
- **Interface utilisateur** (LCD + Keypad)
- **Communication UART** avec l'ESP32
- **Surveillance système** via watchdog
- **Gestion des tâches** avec FreeRTOS

### Framework & OS
- **STM32CubeIDE** / **STM32 HAL**
- **FreeRTOS** pour la gestion des tâches
- **Architecture service/événementielle** avec queues et mutexes

## 🔧 Composants Matériels

| Composant | Interface | Pins | Description |
|-----------|-----------|------|-------------|
| Multiplexeur | GPIO | A0, A1, A2, A3 | Contrôle 4 moteurs |
| Capteurs ToF | I2C2 | SDA=PB11, SCL=PB10 | 5 capteurs de niveau |
| Pins SHUT ToF | GPIO | PB2, PB1, PB15, PB14, PB13 | Activation individuelle |
| LCD | I2C2 | SDA=PB11, SCL=PB10 | Affichage utilisateur |
| Keypad | GPIO | Matrix 4x4 | Interface utilisateur |
| ESP32 | UART1 | RX=PA10, TX=PA9 | Communication inter-cartes |
| Debug | UART2 | RX=PA3, TX=PA2 | Console de débogage |

## 📁 Structure du Projet

```
DPM2_NUCLEO/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── config.h
│   │   ├── global.h
│   │   └── Services/
│   │       ├── motor_service.h
│   │       ├── sensor_stock_service.h
│   │       ├── lcd_service.h
│   │       ├── keypad_service.h
│   │       ├── esp_communication_service.h
│   │       ├── orchestrator.h
│   │       ├── watchdog_service.h
│   │       └── supervision_service.h
│   └── Src/
│       ├── main.c
│       ├── config.c
│       ├── global.c
│       └── Services/
│           ├── motor_service.c
│           ├── sensor_stock_service.c
│           ├── lcd_service.c
│           ├── keypad_service.c
│           ├── esp_communication_service.c
│           ├── orchestrator.c
│           ├── watchdog_service.c
│           └── supervision_service.c
├── test/
│   ├── unity/
│   ├── mocks/
│   └── test_*.c
├── .github/workflows/
│   └── nucleo-ci.yml
└── README.md
```

## 🚀 Démarrage Rapide

### Prérequis
- **STM32CubeIDE** ou **STM32CubeMX**
- **NUCLEO-F411RE**
- **Multiplexeur 4 canaux**
- **5 capteurs ToF VL6180X**
- **LCD I2C**
- **Keypad 4x4**

### Installation

```bash
# Cloner le projet
git clone <https://github.com/Projet-Master-DPM2/DPM2_NUCLEO.git>
cd DPM2_NUCLEO

# Ouvrir dans STM32CubeIDE
# ou compiler avec Make
make -j4
```

### Premier Démarrage

1. **Initialisation** : Le NUCLEO démarre en mode IDLE
2. **Test capteurs** : Vérification des 5 capteurs ToF
3. **Test moteurs** : Test des 4 canaux multiplexeur
4. **Interface** : Affichage LCD et test keypad
5. **Communication** : Attente de connexion ESP32

## 🎮 États du Système

| État | Description | Actions |
|------|-------------|---------|
| `IDLE` | Repos, attente | Affichage menu principal |
| `PAYING` | Mode paiement | Demande scan NFC à ESP32 |
| `ORDERING` | Sélection produit | Interface keypad active |
| `DELIVERING` | Distribution | Contrôle moteurs actif |
| `ERROR` | Erreur système | Affichage erreur, récupération |

## 🔄 Communication UART avec ESP32

### Messages Reçus (ESP32 → NUCLEO)

#### **Commandes de Livraison**
```
ORDER_START:
VEND <product_id> <slot_number> <quantity>
VEND <product_id> <slot_number> <quantity>
...
ORDER_END
```

#### **Messages de Statut**
```
QR_TOKEN_VALID
QR_TOKEN_INVALID
QR_TOKEN_ERROR
QR_TOKEN_BUSY
QR_TOKEN_NO_NETWORK
```

### Messages Envoyés (NUCLEO → ESP32)

#### **Réponses de Livraison**
```
ORDER_ACK
ORDER_NAK
VEND_COMPLETED <product_id> <slot_number>
VEND_FAILED <product_id> <slot_number> <error_code>
DELIVERY_COMPLETED
DELIVERY_FAILED <error_message>
```

#### **Demandes de Service**
```
STATE:PAYING
STATE:IDLE
STATE:ORDERING
STATE:DELIVERING
```

#### **Notifications de Supervision**
```
SUPERVISION_ERROR:{"error_id":"err_123","machine_id":"nucleo_f411re","error_type":"WATCHDOG_RESET","message":"Watchdog reset detected"}
```

## 🔧 Configuration Matérielle

### Multiplexeur (4 Canaux)
```c
// Configuration des slots 1-4
#define SLOT_1_CHANNEL 0
#define SLOT_2_CHANNEL 1
#define SLOT_3_CHANNEL 2
#define SLOT_4_CHANNEL 3

// Validation des slots
#define MIN_SLOT_NUMBER 1
#define MAX_SLOT_NUMBER 4
```

### Capteurs ToF (5 Capteurs)
```c
// Pins SHUT individuels
#define TOF_1_SHUT_PIN GPIO_PIN_2  // PB2
#define TOF_2_SHUT_PIN GPIO_PIN_1  // PB1
#define TOF_3_SHUT_PIN GPIO_PIN_15 // PB15
#define TOF_4_SHUT_PIN GPIO_PIN_14 // PB14
#define TOF_5_SHUT_PIN GPIO_PIN_13 // PB13

// Adresses I2C uniques
#define TOF_1_ADDRESS 0x29
#define TOF_2_ADDRESS 0x30
#define TOF_3_ADDRESS 0x31
#define TOF_4_ADDRESS 0x32
#define TOF_5_ADDRESS 0x33
```

### Interface Utilisateur
```c
// LCD I2C
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// Keypad Matrix 4x4
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
```

## 🔒 Sécurité et Surveillance

### Watchdog
- **IWDG** : Watchdog indépendant pour récupération automatique
- **Détection de blocage** : Surveillance des tâches critiques
- **Récupération** : Reset automatique en cas de problème
- **Logs** : Enregistrement des événements watchdog

### Supervision
- **Service de supervision** : Détection et notification d'erreurs
- **Types d'erreurs** : Watchdog, tâches bloquées, défaillances matérielles
- **Notifications** : Envoi d'erreurs vers ESP32 via UART
- **Rate limiting** : Protection contre le spam (30 secondes)

### Tâches Critiques Surveillées
- **Orchestrator** : Tâche principale de coordination
- **Keypad** : Gestion des entrées utilisateur
- **LCD** : Affichage des informations
- **ESP Communication** : Communication avec ESP32

## 🧪 Tests

### Tests Unitaires

```bash
# Tests natifs (Linux/Mac)
cd test
make test-native

# Tests spécifiques
make test-native-orchestrator
make test-native-motor
make test-native-sensor
make test-native-lcd
```

### Tests Disponibles
- **Orchestrator Logic** : Gestion des états et événements
- **Motor Service** : Contrôle des moteurs et multiplexeur
- **Sensor Stock** : Lecture des capteurs ToF
- **LCD Service** : Affichage et gestion I2C
- **Keypad Service** : Lecture de la matrice
- **ESP Communication** : Protocole UART
- **Global State** : Gestion de l'état global

### Framework de Test
- **Unity** : Framework de test unitaire
- **Mocks** : Simulation des dépendances HAL
- **Tests natifs** : Exécution sur PC pour développement rapide
- **Tests embarqués** : Exécution sur NUCLEO pour validation

## 🚀 CI/CD

### Pipeline GitHub Actions
- ✅ **Tests unitaires natifs** avec rapport JUnit
- ✅ **Build firmware** avec validation
- ✅ **Artefacts** : Génération de fichiers .bin, .elf, .hex
- ✅ **Release automatique** sur tags
- ✅ **Notifications** de statut

### Commandes Locales
```bash
# Tests complets
make test-all

# Build firmware
make -j4

# Analyse statique
make cppcheck

# Validation locale
./scripts/ci-local.sh
```

## 📊 Monitoring

### Logs Runtime
```
[NUCLEO] System startup
[WATCHDOG] Initialized (timeout: 5000ms)
[MOTOR] Multiplexer initialized (4 channels)
[SENSOR] ToF sensors initialized (5 sensors)
[LCD] Display ready (16x2)
[KEYPAD] Matrix initialized (4x4)
[ESP_COMM] UART ready (115200 bps)
[ORCHESTRATOR] Task started
```

### Métriques Système
- **Heap libre** : Surveillance continue
- **Stack usage** : Optimisé par tâche
- **Queue depths** : Monitoring des files d'attente
- **Watchdog resets** : Compteur de récupérations

## 🔧 Configuration

### Pins (config.h)
```c
// Multiplexeur
#define MUX_A0_PIN GPIO_PIN_0  // PA0
#define MUX_A1_PIN GPIO_PIN_1  // PA1
#define MUX_A2_PIN GPIO_PIN_2  // PA2
#define MUX_A3_PIN GPIO_PIN_3  // PA3

// Capteurs ToF SHUT
#define TOF_1_SHUT_PIN GPIO_PIN_2  // PB2
#define TOF_2_SHUT_PIN GPIO_PIN_1  // PB1
#define TOF_3_SHUT_PIN GPIO_PIN_15 // PB15
#define TOF_4_SHUT_PIN GPIO_PIN_14 // PB14
#define TOF_5_SHUT_PIN GPIO_PIN_13 // PB13

// UART
#define UART1_RX_PIN GPIO_PIN_10 // PA10 (ESP32)
#define UART1_TX_PIN GPIO_PIN_9  // PA9
#define UART2_RX_PIN GPIO_PIN_3  // PA3 (Debug)
#define UART2_TX_PIN GPIO_PIN_2  // PA2
```

### Timeouts & Tailles
```c
#define WATCHDOG_TIMEOUT_MS    5000
#define MOTOR_RUN_TIME_MS      2000
#define SENSOR_READ_INTERVAL_MS 1000
#define KEYPAD_DEBOUNCE_MS     50
#define UART_TIMEOUT_MS        1000
```

## 🤝 Intégration ESP32

### Messages Attendus
```
ORDER_START:
VEND prod_123 1 2
VEND prod_456 3 1
ORDER_END
```

### Réponses NUCLEO
```
ORDER_ACK
VEND_COMPLETED prod_123 1
VEND_COMPLETED prod_456 3
DELIVERY_COMPLETED
```

### Gestion d'Erreurs
```
ORDER_NAK
VEND_FAILED prod_123 1 MOTOR_ERROR
DELIVERY_FAILED Sensor malfunction
```

## 📝 Changelog

Voir le [CHANGELOG.md](CHANGELOG.md) pour l'historique détaillé des versions du module NUCLEO.

## 🐛 Dépannage

### Problèmes Courants

**Moteurs ne fonctionnent pas :**
- Vérifier câblage multiplexeur
- Contrôler alimentation moteurs
- Tester avec commande directe

**Capteurs ToF ne répondent pas :**
- Vérifier pins SHUT individuels
- Contrôler adresses I2C uniques
- Tester communication I2C

**UART silencieux :**
- Vérifier connexions RX/TX croisées
- Contrôler baudrate (115200)
- Tester avec ESP32 connecté

**Watchdog reset fréquent :**
- Vérifier tâches critiques
- Contrôler stack sizes
- Analyser logs de supervision

### Debug Avancé

```c
// Dans config.h, activer debug
#define DEBUG_MODE 1
#define WATCHDOG_DEBUG 1

// Monitoring heap
#define HEAP_MONITOR_ENABLED 1
```

## 📄 Licence

Ce projet fait partie du cursus académique M2 - Sophia Ynov Campus
Réalisé par l'équipe DPM - Distributeur Projet Master

---

**Équipe DPM2** - Distributeur Automatique Intelligent  
*NUCLEO-F411RE Control Module - v2.0*
