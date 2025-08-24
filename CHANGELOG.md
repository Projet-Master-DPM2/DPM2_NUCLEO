# Changelog - DPM2 NUCLEO

Toutes les modifications notables du projet NUCLEO-F411RE seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/lang/fr/).

## [2.0.0] - 2025-08-XX

### 🎯 **Ajouté - Système de Supervision**

- **Service de supervision** : Nouveau service pour détecter et notifier les erreurs critiques
- **Notifications UART** : Envoi d'erreurs vers ESP32 via UART
- **Intégration watchdog** : Notifications pour les resets watchdog, tâches bloquées, et défaillances FreeRTOS
- **Rate limiting** : Protection contre le spam (30 secondes entre notifications)
- **ID machine unique** : Configuration pour identification NUCLEO
- **Types d'erreurs étendus** : Support pour erreurs moteur et capteur

### 🏗️ **Modifié - Configuration Matérielle**

- **Multiplexeur** : Réduction de 8 à 4 canaux de sortie (1 à 4)
- **Capteurs ToF** : Passage de 4 à 5 capteurs avec pins SHUT individuels
- **Pins SHUT** : PB2, PB1, PB15, PB14, PB13 (dans l'ordre)
- **Validation des slots** : Mise à jour pour accepter uniquement les slots 1-4
- **Service moteur** : Adaptation pour la nouvelle configuration multiplexeur
- **Service capteurs** : Support pour 5 capteurs ToF avec adresses I2C uniques

### 🔧 **Corrigé - Compilation et Intégration**

- **HardFault_Handler** : Suppression de la redéfinition et intégration de la gestion d'erreurs
- **printf** : Ajout de `#include <stdio.h>` dans main.c
- **Commentaires** : Correction des commentaires malformés dans gpio.c
- **Variables HardFault** : Déplacement des déclarations vers la section appropriée
- **snprintf** : Ajout de vérification de taille pour éviter la troncature
- **TaskHealth_t** : Remplacement de `isCritical` par vérification explicite des tâches critiques
- **Compilation réussie** : Tous les fichiers compilent sans erreur

### 📚 **Ajouté - Documentation**

#### **Documentation Technique**
- **SUPERVISION_SYSTEM.md** : Documentation complète du système de supervision
- **SUPERVISION_COMPILATION_FIXES.md** : Corrections de compilation pour NUCLEO
- **HARDWARE_CONFIG.md** : Configuration matérielle mise à jour
- **UART_PROTOCOL.md** : Documentation du protocole de communication UART

#### **Configuration**
- **Configuration matérielle** : Documentation des nouvelles pins et capteurs
- **Validation des slots** : Mise à jour pour la nouvelle configuration

## [1.0.0] - 2025-07-XX

### 🎯 **Ajouté - Fonctionnalités de Base**

- **Contrôle moteur** : Gestion des moteurs via multiplexeur
- **Capteurs de stock** : Détection de niveau via capteurs ToF
- **Communication UART** : Réception et traitement des commandes ESP32
- **Interface utilisateur** : LCD et keypad
- **Watchdog** : Surveillance système et récupération automatique
- **FreeRTOS** : Gestion des tâches et synchronisation
- **Sécurité** : Validation des entrées, gestion des erreurs

### 🏗️ **Ajouté - Infrastructure**

#### **CI/CD Pipeline**
- **GitHub Actions** : Automatisation complète des builds et tests
- **Tests unitaires** : Framework Unity pour tests natifs et embarqués
- **Analyse statique** : Cppcheck pour détection d'erreurs
- **Notifications** : Emails automatiques avec résultats de tests
- **Artefacts** : Génération de firmwares prêts à flasher

#### **Tests et Qualité**
- **Tests unitaires** : Couverture complète des services critiques
- **Mocks** : Simulation des dépendances matérielles
- **Validation** : Tests d'intégration et de workflow
- **Documentation** : Guides d'utilisation et API

### 🔒 **Ajouté - Sécurité**

#### **Analyse OWASP**
- **Vulnérabilités critiques** : Identification et correction
- **Validation d'entrée** : Protection contre les injections
- **Gestion d'erreurs** : Sans fuite d'informations
- **Watchdog** : Protection contre les blocages système
- **Logging sécurisé** : Protection des informations sensibles

### 📚 **Ajouté - Documentation**

#### **Documentation Technique**
- **README.md** : Documentation complète du projet
- **Configuration** : Guides d'installation et configuration
- **API** : Documentation des endpoints et protocoles
- **Sécurité** : Rapport d'analyse OWASP et mesures
- **Tests** : Documentation des tests et couverture

---

## Format des Versions

- **MAJOR** : Changements incompatibles avec les versions précédentes
- **MINOR** : Nouvelles fonctionnalités compatibles
- **PATCH** : Corrections de bugs compatibles

## Types de Changements

- **Ajouté** : Nouvelles fonctionnalités
- **Modifié** : Changements dans les fonctionnalités existantes
- **Déprécié** : Fonctionnalités qui seront supprimées
- **Supprimé** : Fonctionnalités supprimées
- **Corrigé** : Corrections de bugs
- **Sécurité** : Corrections de vulnérabilités
