# Makefile pour Tests Unitaires DPM2_NUCLEO
# Supporte tests natifs (PC) et embarqués (STM32)

# ============================================================================
# CONFIGURATION
# ============================================================================

# Compilateurs
CC_NATIVE = gcc
CC_EMBEDDED = arm-none-eabi-gcc

# Répertoires
PROJECT_ROOT = ..
CORE_DIR = $(PROJECT_ROOT)/Core
TEST_DIR = .
UNITY_DIR = $(TEST_DIR)/unity
MOCKS_DIR = $(TEST_DIR)/mocks
NATIVE_DIR = $(TEST_DIR)/native
EMBEDDED_DIR = $(TEST_DIR)/embedded

# Répertoires de build
BUILD_DIR = build
BUILD_NATIVE_DIR = $(BUILD_DIR)/native
BUILD_EMBEDDED_DIR = $(BUILD_DIR)/embedded

# Répertoires de sortie
REPORTS_DIR = reports

# ============================================================================
# FLAGS DE COMPILATION
# ============================================================================

# Flags communs
COMMON_CFLAGS = -Wall -Wextra -std=c99 -g -O0

# Flags pour tests natifs (PC)
NATIVE_CFLAGS = $(COMMON_CFLAGS) \
	-DUNITY_NATIVE_TESTS \
	-I$(UNITY_DIR) \
	-I$(MOCKS_DIR) \
	-I$(CORE_DIR)/Inc \
	-I$(CORE_DIR)/Inc/Services

# Flags pour tests embarqués (STM32)
EMBEDDED_CFLAGS = $(COMMON_CFLAGS) \
	-mcpu=cortex-m4 \
	-mthumb \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-DSTM32F411xE \
	-DUSE_HAL_DRIVER \
	-I$(UNITY_DIR) \
	-I$(CORE_DIR)/Inc \
	-I$(CORE_DIR)/Inc/Services \
	-I$(PROJECT_ROOT)/Drivers/STM32F4xx_HAL_Driver/Inc \
	-I$(PROJECT_ROOT)/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
	-I$(PROJECT_ROOT)/Drivers/CMSIS/Include

# Flags de linkage natifs
NATIVE_LDFLAGS = -lm

# Flags de linkage embarqués
EMBEDDED_LDFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	-T$(PROJECT_ROOT)/STM32F411RETX_FLASH.ld \
	-Wl,--gc-sections \
	-static \
	--specs=nano.specs \
	-Wl,--start-group -lc -lm -Wl,--end-group

# ============================================================================
# SOURCES
# ============================================================================

# Sources Unity
UNITY_SOURCES = \
	$(UNITY_DIR)/unity.c \
	$(UNITY_DIR)/unity.h

# Sources mocks (pour tests natifs uniquement)
MOCK_SOURCES = \
	$(MOCKS_DIR)/mock_hal.c \
	$(MOCKS_DIR)/mock_freertos.c \
	$(MOCKS_DIR)/mock_watchdog_service.c \
	$(MOCKS_DIR)/mock_global.c

# Sources du projet (code à tester)
PROJECT_SOURCES =

# Tests natifs
NATIVE_TESTS = \
	$(NATIVE_DIR)/test_orchestrator/test_orchestrator_logic.c \
	$(NATIVE_DIR)/test_watchdog/test_watchdog_logic.c \
	$(NATIVE_DIR)/test_global_state/test_global_state.c \
	$(NATIVE_DIR)/test_motor_service/test_motor_service_logic.c \
	$(NATIVE_DIR)/test_keypad_service/test_keypad_service_logic.c \
	$(NATIVE_DIR)/test_esp_comm_service/test_esp_comm_service_logic.c \
	$(NATIVE_DIR)/test_lcd_service/test_lcd_service_logic.c \
	$(NATIVE_DIR)/test_sensor_stock_service/test_sensor_stock_service_logic.c

# Tests embarqués
EMBEDDED_TESTS = \
	$(EMBEDDED_DIR)/test_hardware/test_i2c_hardware.c \
	$(EMBEDDED_DIR)/test_freertos/test_freertos_integration.c

# ============================================================================
# RÈGLES PRINCIPALES
# ============================================================================

.PHONY: all clean test-native test-embedded test-all help

all: test-native

help:
	@echo "Tests Unitaires DPM2_NUCLEO"
	@echo "============================"
	@echo ""
	@echo "Cibles disponibles:"
	@echo "  test-native     - Exécuter tests natifs (PC)"
	@echo "  test-embedded   - Compiler tests embarqués (STM32)"
	@echo "  test-all        - Exécuter tous les tests"
	@echo "  clean           - Nettoyer les fichiers de build"
	@echo "  reports         - Générer rapports de tests"
	@echo "  help            - Afficher cette aide"
	@echo ""
	@echo "Variables:"
	@echo "  VERBOSE=1       - Affichage verbeux"
	@echo "  COVERAGE=1      - Activer couverture de code"

# ============================================================================
# TESTS NATIFS (PC)
# ============================================================================

test-native: $(BUILD_NATIVE_DIR) $(REPORTS_DIR)
	@echo "=== TESTS NATIFS ==="
	@for test in $(NATIVE_TESTS); do \
		test_name=$$(basename $$test .c); \
		test_exe=$(BUILD_NATIVE_DIR)/$$test_name; \
		echo "Compilation: $$test_name"; \
		$(CC_NATIVE) $(NATIVE_CFLAGS) \
			$$test \
			$(UNITY_DIR)/unity.c \
			$(MOCK_SOURCES) \
			$(PROJECT_SOURCES) \
			$(NATIVE_LDFLAGS) \
			-o $$test_exe; \
		if [ $$? -eq 0 ]; then \
			echo "Exécution: $$test_name"; \
			$$test_exe > $(REPORTS_DIR)/$$test_name.log 2>&1; \
			if [ $$? -eq 0 ]; then \
				echo "✅ $$test_name: SUCCÈS"; \
			else \
				echo "❌ $$test_name: ÉCHEC"; \
				cat $(REPORTS_DIR)/$$test_name.log; \
			fi; \
		else \
			echo "❌ $$test_name: ERREUR DE COMPILATION"; \
		fi; \
	done

# Test individuel natif
test-native-%: $(BUILD_NATIVE_DIR) $(REPORTS_DIR)
	@if [ "$*" = "global_state" ]; then \
		test_file=$(NATIVE_DIR)/test_$*/test_$*.c; \
		test_exe=$(BUILD_NATIVE_DIR)/test_$*; \
	else \
		test_file=$(NATIVE_DIR)/test_$*/test_$*_logic.c; \
		test_exe=$(BUILD_NATIVE_DIR)/test_$*_logic; \
	fi; \
	echo "=== TEST NATIF: $* ==="; \
	$(CC_NATIVE) $(NATIVE_CFLAGS) \
		$$test_file \
		$(UNITY_DIR)/unity.c \
		$(MOCK_SOURCES) \
		$(PROJECT_SOURCES) \
		$(NATIVE_LDFLAGS) \
		-o $$test_exe; \
	if $$test_exe; then \
		echo "✅ $*_logic: SUCCÈS"; \
	else \
		echo "❌ $*_logic: ÉCHEC"; \
	fi

# ============================================================================
# TESTS EMBARQUÉS (STM32)
# ============================================================================

test-embedded: $(BUILD_EMBEDDED_DIR)
	@echo "=== TESTS EMBARQUÉS ==="
	@echo "⚠️  Tests embarqués nécessitent une carte STM32 connectée"
	@for test in $(EMBEDDED_TESTS); do \
		test_name=$$(basename $$test .c); \
		test_elf=$(BUILD_EMBEDDED_DIR)/$$test_name.elf; \
		echo "Compilation: $$test_name"; \
		$(CC_EMBEDDED) $(EMBEDDED_CFLAGS) \
			$$test \
			$(UNITY_SOURCES) \
			$(PROJECT_SOURCES) \
			$(EMBEDDED_LDFLAGS) \
			-o $$test_elf; \
		if [ $$? -eq 0 ]; then \
			echo "✅ $$test_name: COMPILATION OK"; \
			# TODO: Flash et exécution sur cible via OpenOCD/ST-Link
		else \
			echo "❌ $$test_name: ERREUR DE COMPILATION"; \
		fi; \
	done

# ============================================================================
# COUVERTURE DE CODE
# ============================================================================

coverage: NATIVE_CFLAGS += --coverage
coverage: NATIVE_LDFLAGS += --coverage
coverage: test-native
	@echo "=== COUVERTURE DE CODE ==="
	@mkdir -p $(REPORTS_DIR)/coverage
	@gcov $(BUILD_NATIVE_DIR)/*.gcno
	@mv *.gcov $(REPORTS_DIR)/coverage/ 2>/dev/null || true
	@echo "Rapport de couverture généré dans $(REPORTS_DIR)/coverage/"

# ============================================================================
# RAPPORTS ET ANALYSE
# ============================================================================

test-reports: $(REPORTS_DIR)
	@echo "=== GÉNÉRATION DES RAPPORTS ==="
	@echo "Analyse des logs de tests..."
	@total=0; passed=0; failed=0; \
	for log in $(REPORTS_DIR)/*.log; do \
		if [ -f "$$log" ]; then \
			tests=$$(grep -c "RUN_TEST" "$$log" 2>/dev/null || echo "0"); \
			failures=$$(grep -c "FAIL:" "$$log" 2>/dev/null || echo "0"); \
			total=$$((total + tests)); \
			failed=$$((failed + failures)); \
			passed=$$((passed + tests - failures)); \
		fi; \
	done; \
	echo ""; \
	echo "=== RÉSUMÉ DES TESTS ==="; \
	echo "Total:   $$total"; \
	echo "Réussis: $$passed"; \
	echo "Échoués: $$failed"; \
	if [ "$$failed" -eq 0 ]; then \
		echo "✅ Tous les tests sont passés!"; \
	else \
		echo "❌ $$failed test(s) ont échoué"; \
	fi

# ============================================================================
# ANALYSE STATIQUE
# ============================================================================

static-analysis:
	@echo "=== ANALYSE STATIQUE ==="
	@which cppcheck > /dev/null || (echo "❌ cppcheck non installé" && exit 1)
	cppcheck --enable=all --std=c99 \
		-I$(CORE_DIR)/Inc \
		-I$(CORE_DIR)/Inc/Services \
		--suppress=missingIncludeSystem \
		--xml --xml-version=2 \
		$(CORE_DIR)/Src/ \
		2> $(REPORTS_DIR)/cppcheck.xml
	@echo "✅ Analyse statique terminée: $(REPORTS_DIR)/cppcheck.xml"

# ============================================================================
# UTILITAIRES
# ============================================================================

# Créer répertoires de build
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_NATIVE_DIR): $(BUILD_DIR)
	@mkdir -p $(BUILD_NATIVE_DIR)

$(BUILD_EMBEDDED_DIR): $(BUILD_DIR)
	@mkdir -p $(BUILD_EMBEDDED_DIR)

$(REPORTS_DIR):
	@mkdir -p $(REPORTS_DIR)

# Nettoyage
clean:
	@echo "Nettoyage des fichiers de build..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(REPORTS_DIR)
	@rm -f *.gcov *.gcda *.gcno
	@echo "✅ Nettoyage terminé"

# Tests complets
test-all: test-native test-embedded test-reports

# Mode verbeux
ifeq ($(VERBOSE),1)
	NATIVE_CFLAGS += -DUNITY_VERBOSE
	EMBEDDED_CFLAGS += -DUNITY_VERBOSE
endif

# Validation du Makefile
validate:
	@echo "=== VALIDATION DE LA CONFIGURATION ==="
	@echo "Compilateur natif: $(shell which $(CC_NATIVE) || echo 'NON TROUVÉ')"
	@echo "Compilateur embarqué: $(shell which $(CC_EMBEDDED) || echo 'NON TROUVÉ')"
	@echo "Tests natifs: $(words $(NATIVE_TESTS)) fichiers"
	@echo "Tests embarqués: $(words $(EMBEDDED_TESTS)) fichiers"
	@echo "Sources projet: $(words $(PROJECT_SOURCES)) fichiers"
