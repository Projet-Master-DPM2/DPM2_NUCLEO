# Tests Unitaires - DPM2_NUCLEO

## Structure des Tests

```
test/
├── native/                     # Tests exécutables sur PC
│   ├── test_orchestrator/      # Tests logique orchestrateur
│   ├── test_watchdog/          # Tests logique watchdog
│   ├── test_global_state/      # Tests gestion état global
│   └── test_utils/             # Tests utilitaires
├── embedded/                   # Tests sur cible STM32
│   ├── test_hardware/          # Tests matériels (I2C, UART, GPIO)
│   ├── test_freertos/          # Tests intégration FreeRTOS
│   └── test_integration/       # Tests d'intégration complète
├── mocks/                      # Mocks pour HAL et FreeRTOS
├── unity/                      # Framework Unity
└── scripts/                    # Scripts d'automation
```

## Types de Tests

### Tests Natifs (PC)
- **Logique métier pure** (orchestrateur, état machine)
- **Algorithmes** (validation, parsing)
- **Structures de données**
- **Fonctions utilitaires**

### Tests Embarqués (STM32)
- **Drivers matériels** (I2C, UART, GPIO)
- **Intégration FreeRTOS** (tâches, queues, mutex)
- **Tests système** complets
- **Performance** et timing

## Exécution

```bash
# Tests natifs
make test-native

# Tests embarqués
make test-embedded

# Tous les tests
make test-all
```
