# Mise à jour de la Configuration Matérielle NUCLEO

## Vue d'ensemble

Ce document décrit les modifications apportées au projet NUCLEO pour s'adapter à la nouvelle configuration matérielle avec un multiplexeur 4 channels et 5 capteurs ToF.

## Changements de Configuration

### 1. **Multiplexeur - Réduction à 4 channels**

#### **Configuration précédente :**
- Multiplexeur avec jusqu'à 16 channels (0-15)
- Validation des slots : 1-99

#### **Nouvelle configuration :**
- Multiplexeur limité à 4 channels (1-4)
- Validation des slots : 1-4 uniquement

#### **Mapping des commandes :**
```c
// Ancien mapping (0-6)
case 11: return 0;  // Channel 0
case 12: return 1;  // Channel 1
case 13: return 2;  // Channel 2
case 21: return 3;  // Channel 3
case 22: return 4;  // Channel 4
case 23: return 6;  // Channel 6

// Nouveau mapping (1-4)
case 11: return 1;  // Channel 1 du multiplexeur
case 12: return 2;  // Channel 2 du multiplexeur
case 13: return 3;  // Channel 3 du multiplexeur
case 21: return 4;  // Channel 4 du multiplexeur
case 22: return 1;  // Retour au channel 1 (rotation)
case 23: return 2;  // Retour au channel 2 (rotation)
```

### 2. **Capteurs ToF - Extension à 5 capteurs**

#### **Configuration précédente :**
- 1 capteur ToF VL6180X
- Broche SHUT : PB1 uniquement
- Adresse I2C : 0x29 (défaut)

#### **Nouvelle configuration :**
- 5 capteurs ToF VL6180X
- Broches SHUT individuelles :
  - Capteur 1 : PB2
  - Capteur 2 : PB1
  - Capteur 3 : PB15
  - Capteur 4 : PB14
  - Capteur 5 : PB13
- Adresses I2C uniques :
  - Capteur 1 : 0x29 (0x52 en 8-bit)
  - Capteur 2 : 0x2A (0x54 en 8-bit)
  - Capteur 3 : 0x2B (0x56 en 8-bit)
  - Capteur 4 : 0x2C (0x58 en 8-bit)
  - Capteur 5 : 0x2D (0x5A en 8-bit)

## Modifications de Code

### 1. **Service Moteur (`motor_service.c`)**

#### **Fonction `MotorService_OrderToChannel()` :**
- Mapping mis à jour pour utiliser les channels 1-4
- Rotation des commandes 22 et 23 vers les channels 1-2

### 2. **Service de Communication ESP (`esp_communication_service.c`)**

#### **Validation des paramètres :**
```c
// Ancien : validation 1-99
if (parsed_slot >= 1 && parsed_slot <= 99 && parsed_qty >= 1 && parsed_qty <= 10)

// Nouveau : validation 1-4
if (parsed_slot >= 1 && parsed_slot <= 4 && parsed_qty >= 1 && parsed_qty <= 10)
```

#### **Validation dans l'orchestrateur :**
```c
// Ancien : validation 1-99
if (channel >= 1 && channel <= 99)

// Nouveau : validation 1-4
if (channel >= 1 && channel <= 4)
```

### 3. **Service de Capteurs (`sensor_stock_service.c`)**

#### **Configuration des capteurs :**
```c
// Ancien : 1 capteur
TofSensorCfg sensors[1] = {
    { .id = 0, .i2cAddr8 = VL6180_DEFAULT_ADDR_8BIT, 
      .shutPort = TOF_SHUT_GPIO_Port, .shutPin = TOF_SHUT_Pin, 
      .thresholdMm = 170 }
};

// Nouveau : 5 capteurs
TofSensorCfg sensors[5] = {
    { .id = 0, .i2cAddr8 = (0x29 << 1), 
      .shutPort = TOF_SHUT_1_GPIO_Port, .shutPin = TOF_SHUT_1_Pin, 
      .thresholdMm = 170 },
    { .id = 1, .i2cAddr8 = (0x2A << 1), 
      .shutPort = TOF_SHUT_2_GPIO_Port, .shutPin = TOF_SHUT_2_Pin, 
      .thresholdMm = 170 },
    // ... 3 autres capteurs
};
```

#### **Boucle de lecture :**
```c
// Ancien : lecture d'1 capteur
for (uint8_t i = 0; i < 1; ++i)

// Nouveau : lecture de 5 capteurs
for (uint8_t i = 0; i < 5; ++i)
```

### 4. **En-tête des Capteurs (`sensor_stock_service.h`)**

#### **Nouvelles définitions de broches :**
```c
// Ancien : 1 broche SHUT
#define TOF_SHUT_GPIO_Port     GPIOB
#define TOF_SHUT_Pin           GPIO_PIN_1

// Nouveau : 5 broches SHUT
#define TOF_SHUT_1_GPIO_Port   GPIOB
#define TOF_SHUT_1_Pin         GPIO_PIN_2
#define TOF_SHUT_2_GPIO_Port   GPIOB
#define TOF_SHUT_2_Pin         GPIO_PIN_1
// ... 3 autres définitions
```

## Impact sur le Protocole UART

### **Validation des commandes VEND :**
- Les slots 1-4 sont maintenant les seuls valides
- Les slots 5+ génèrent une erreur `VEND_FAILED:<slot>:INVALID_CHANNEL`

### **Messages d'erreur mis à jour :**
- Documentation mise à jour pour préciser la limitation 1-4
- Messages d'erreur plus précis

## Avantages de la Nouvelle Configuration

### **Multiplexeur 4 channels :**
- **Simplicité** : Moins de complexité dans la sélection des channels
- **Fiabilité** : Moins de risques d'erreur de sélection
- **Performance** : Temps de stabilisation réduit

### **5 capteurs ToF :**
- **Couverture étendue** : Plus de zones de détection de stock
- **Redondance** : Meilleure fiabilité de détection
- **Flexibilité** : Possibilité de détecter différents types de produits

## Compatibilité

### **Avec l'ESP32 :**
- Le protocole UART reste compatible
- Seule la validation des slots change (1-4 au lieu de 1-99)
- Les commandes existantes continuent de fonctionner

### **Avec le système keypad :**
- Le mapping des commandes keypad reste fonctionnel
- Rotation automatique pour les commandes 22 et 23

## Tests Recommandés

1. **Test du multiplexeur :**
   - Vérifier que seuls les channels 1-4 sont accessibles
   - Tester la rotation des commandes 22 et 23

2. **Test des capteurs ToF :**
   - Vérifier l'initialisation des 5 capteurs
   - Tester la lecture de distance sur chaque capteur
   - Vérifier les adresses I2C uniques

3. **Test de communication :**
   - Envoyer des commandes VEND avec slots 1-4
   - Vérifier les erreurs pour slots > 4
   - Tester le workflow complet de livraison
