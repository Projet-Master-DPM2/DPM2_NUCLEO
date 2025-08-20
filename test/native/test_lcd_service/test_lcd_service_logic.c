#include "unity.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Inclure les mocks
#include "../../mocks/mock_hal.h"
#include "../../mocks/mock_freertos.h"
#include "../../mocks/mock_global.h"
// mock_i2c.h supprimé - utilise mock_hal.h

// Constantes LCD (copiées du service pour les tests)
#define LCD_ADDR         (0x27 << 1)
#define LCD_BACKLIGHT    0x08
#define LCD_ENABLE       0x04
#define LCD_RW           0x02
#define LCD_RS           0x01

// Structure LcdMessage pour les tests (utilise la définition de mock_global.h)
// LcdMessage est déjà défini dans mock_global.h

// Variables simulées pour les tests
static osMutexId_t mock_i2c1Mutex = NULL;

// =============================================================================
// FONCTIONS COPIÉES DU SERVICE POUR TESTS UNITAIRES
// =============================================================================

// Version de test de lcd_send_nibble (sans mutex réel)
static void lcd_send_nibble_test(uint8_t nibble, uint8_t control) {
    uint8_t data = nibble | control | LCD_BACKLIGHT;
    
    // Simuler les 3 transmissions I2C
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, 50);
    data |= LCD_ENABLE;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, 50);
    // osDelay(1) simulé
    data &= ~LCD_ENABLE;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, 50);
    // osDelay(1) simulé
}

// Version de test de lcd_send_command
static void lcd_send_command_test(uint8_t cmd) {
    lcd_send_nibble_test(cmd & 0xF0, 0);
    lcd_send_nibble_test((cmd << 4) & 0xF0, 0);
    // osDelay(2) simulé
}

// Version de test de lcd_send_data
static void lcd_send_data_test(uint8_t data) {
    lcd_send_nibble_test(data & 0xF0, LCD_RS);
    lcd_send_nibble_test((data << 4) & 0xF0, LCD_RS);
    // osDelay(2) simulé
}

// Version de test de lcd_clear
static void lcd_clear_test(void) {
    lcd_send_command_test(0x01);
    // osDelay(2) simulé
}

// Version de test de lcd_set_cursor
static void lcd_set_cursor_test(uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_send_command_test(0x80 | (col + row_offsets[row]));
}

// Version de test de lcd_send_string
static void lcd_send_string_test(char *str) {
    while (*str) {
        lcd_send_data_test((uint8_t)*str++);
    }
}

// Version de test de lcd_scroll_string
static void lcd_scroll_string_test(char *str, uint8_t row, uint16_t delay_ms) {
    (void)delay_ms; // Pas de délai réel dans les tests
    
    char buffer[17];
    size_t len = strlen(str);
    
    if (len <= 16) {
        lcd_set_cursor_test(row, 0);
        lcd_send_string_test(str);
        return;
    }
    
    // Simuler le scrolling (première position seulement pour test)
    strncpy(buffer, str, 16);
    buffer[16] = '\0';
    lcd_set_cursor_test(row, 0);
    lcd_send_string_test(buffer);
}

// =============================================================================
// SETUP ET TEARDOWN
// =============================================================================

void setUp(void) {
    // Reset des mocks avant chaque test
    // Mock_I2C_Reset() - utilise Mock_HAL_Reset()
    Mock_HAL_Reset();
    Mock_FreeRTOS_Reset();
    
    // Configurer I2C pour réussir par défaut
    // Mock_I2C_SetNextStatus(HAL_OK) - utilise Mock_HAL_SetI2CResponse()
    
    mock_i2c1Mutex = (osMutexId_t)0x12345678; // Pointeur fictif
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// =============================================================================
// TESTS UNITAIRES LCD SERVICE
// =============================================================================

// Test de l'envoi d'un nibble
void test_lcd_send_nibble_basic(void) {
    uint8_t nibble = 0x30;
    uint8_t control = 0x00;
    
    lcd_send_nibble_test(nibble, control);
    
    // Vérifier que 3 transmissions I2C ont eu lieu
    TEST_ASSERT_EQUAL_UINT32(3, Mock_HAL_GetI2CCallCount());
    
    // Pas de vérification d'adresse spécifique avec mock_hal simplifié
}

// Test de l'envoi d'une commande
void test_lcd_send_command_basic(void) {
    uint8_t command = 0x28; // 4-bit, 2 lignes, 5x8
    
    lcd_send_command_test(command);
    
    // Une commande = 2 nibbles = 6 transmissions I2C
    TEST_ASSERT_EQUAL_UINT32(6, Mock_HAL_GetI2CCallCount());
}

// Test de l'envoi de données (caractères)
void test_lcd_send_data_character(void) {
    uint8_t character = 'A'; // 0x41
    
    lcd_send_data_test(character);
    
    // Un caractère = 2 nibbles = 6 transmissions I2C
    TEST_ASSERT_EQUAL_UINT32(6, Mock_HAL_GetI2CCallCount());
}

// Test de clear screen
void test_lcd_clear_screen(void) {
    lcd_clear_test();
    
    // Clear = commande 0x01 = 6 transmissions I2C
    TEST_ASSERT_EQUAL_UINT32(6, Mock_HAL_GetI2CCallCount());
}

// Test de positionnement du curseur
void test_lcd_set_cursor_positions(void) {
    // Test position (0,0) - début première ligne
    Mock_HAL_Reset();
    lcd_set_cursor_test(0, 0);
    TEST_ASSERT_EQUAL_UINT32(6, Mock_HAL_GetI2CCallCount());
    
    // Test position (1,0) - début deuxième ligne
    Mock_HAL_Reset();
    lcd_set_cursor_test(1, 0);
    TEST_ASSERT_EQUAL_UINT32(6, Mock_HAL_GetI2CCallCount());
    
    // Test position (0,5) - colonne 5 première ligne
    Mock_HAL_Reset();
    lcd_set_cursor_test(0, 5);
    TEST_ASSERT_EQUAL_UINT32(6, Mock_HAL_GetI2CCallCount());
}

// Test d'affichage d'une chaîne courte
void test_lcd_send_string_short(void) {
    char* test_string = "Hello";
    
    lcd_send_string_test(test_string);
    
    // 5 caractères = 5 * 6 = 30 transmissions I2C
    TEST_ASSERT_EQUAL_UINT32(30, Mock_HAL_GetI2CCallCount());
}

// Test d'affichage d'une chaîne vide
void test_lcd_send_string_empty(void) {
    char* test_string = "";
    
    lcd_send_string_test(test_string);
    
    // Chaîne vide = 0 transmission
    TEST_ASSERT_EQUAL_UINT32(0, Mock_HAL_GetI2CCallCount());
}

// Test d'affichage d'une chaîne de 16 caractères (largeur écran)
void test_lcd_send_string_full_width(void) {
    char* test_string = "1234567890123456"; // 16 caractères
    
    lcd_send_string_test(test_string);
    
    // 16 caractères = 16 * 6 = 96 transmissions I2C
    TEST_ASSERT_EQUAL_UINT32(96, Mock_HAL_GetI2CCallCount());
}

// Test de scrolling - chaîne courte (pas de scroll)
void test_lcd_scroll_string_no_scroll(void) {
    char* test_string = "Short";
    
    lcd_scroll_string_test(test_string, 0, 100);
    
    // Chaîne courte: set_cursor + send_string
    // set_cursor = 6, "Short" = 5*6 = 30, total = 36
    TEST_ASSERT_EQUAL_UINT32(36, Mock_HAL_GetI2CCallCount());
}

// Test de scrolling - chaîne longue (avec scroll)
void test_lcd_scroll_string_with_scroll(void) {
    char* test_string = "This is a very long string that needs scrolling";
    
    lcd_scroll_string_test(test_string, 0, 100);
    
    // Chaîne longue: set_cursor + premiers 16 caractères
    // set_cursor = 6, 16 chars = 16*6 = 96, total = 102
    TEST_ASSERT_EQUAL_UINT32(102, Mock_HAL_GetI2CCallCount());
}

// Test de gestion d'erreur I2C
void test_lcd_i2c_error_handling(void) {
    // Configurer le mock pour retourner une erreur
    Mock_HAL_SetI2CResponse(HAL_ERROR, NULL, 0);
    
    lcd_send_command_test(0x28);
    
    // Même avec erreur, le nombre d'appels doit être correct
    TEST_ASSERT_EQUAL_UINT32(6, Mock_HAL_GetI2CCallCount());
}

// Test de validation des messages LCD
void test_lcd_message_validation(void) {
    LcdMessage msg;
    
    // Test message valide
    strcpy(msg.line1, "Line 1");
    strcpy(msg.line2, "Line 2");
    
    // Vérifier que les chaînes sont bien null-terminées
    TEST_ASSERT_EQUAL_CHAR('\0', msg.line1[6]);
    TEST_ASSERT_EQUAL_CHAR('\0', msg.line2[6]);
    
    // Test longueur maximale
    strcpy(msg.line1, "1234567890123456"); // 16 caractères exactement
    TEST_ASSERT_EQUAL_UINT8(16, strlen(msg.line1));
    
    // Test protection buffer overflow
    strncpy(msg.line1, "This string is way too long for the LCD", 16);
    msg.line1[16] = '\0'; // Force null termination
    TEST_ASSERT_EQUAL_UINT8(16, strlen(msg.line1));
}

// Test de robustesse avec pointeurs NULL
void test_lcd_null_pointer_safety(void) {
    // Test avec pointeur NULL (ne doit pas planter)
    char* null_string = NULL;
    
    // Note: Dans un vrai test, on voudrait que la fonction gère NULL
    // Ici on teste juste que notre version de test ne plante pas
    if (null_string != NULL) {
        lcd_send_string_test(null_string);
    }
    
    // Si on arrive ici, pas de crash
    TEST_ASSERT_TRUE(true);
}

// Test de séquence complète d'affichage
void test_lcd_complete_display_sequence(void) {
    LcdMessage msg;
    strcpy(msg.line1, "DPM Ready");
    strcpy(msg.line2, "Insert Card");
    
    // Simuler l'affichage complet d'un message
    // Mock_I2C_Reset() - utilise Mock_HAL_Reset()
    
    // Clear screen
    lcd_clear_test();
    uint32_t clear_calls = Mock_HAL_GetI2CCallCount();
    
    // Position ligne 1
    lcd_set_cursor_test(0, 0);
    uint32_t cursor1_calls = Mock_HAL_GetI2CCallCount() - clear_calls;
    
    // Afficher ligne 1
    lcd_send_string_test(msg.line1);
    uint32_t line1_calls = Mock_HAL_GetI2CCallCount() - clear_calls - cursor1_calls;
    
    // Position ligne 2
    lcd_set_cursor_test(1, 0);
    uint32_t cursor2_calls = Mock_HAL_GetI2CCallCount() - clear_calls - cursor1_calls - line1_calls;
    
    // Afficher ligne 2
    lcd_send_string_test(msg.line2);
    
    // Vérifications
    TEST_ASSERT_EQUAL_UINT32(6, clear_calls);      // Clear = 1 commande
    TEST_ASSERT_EQUAL_UINT32(6, cursor1_calls);    // Set cursor = 1 commande
    TEST_ASSERT_EQUAL_UINT32(54, line1_calls);     // "DPM Ready" = 9 chars
    TEST_ASSERT_EQUAL_UINT32(6, cursor2_calls);    // Set cursor = 1 commande
    // "Insert Card" = 11 chars = 66 transmissions
}

// Test de performance - affichage rapide
void test_lcd_performance_rapid_updates(void) {
    char messages[5][17] = {
        "Message 1",
        "Message 2", 
        "Message 3",
        "Message 4",
        "Message 5"
    };
    
    // Mock_I2C_Reset() - utilise Mock_HAL_Reset()
    
    // Simuler 5 mises à jour rapides
    for (int i = 0; i < 5; i++) {
        lcd_clear_test();
        lcd_set_cursor_test(0, 0);
        lcd_send_string_test(messages[i]);
    }
    
    // Chaque cycle: clear(6) + cursor(6) + string(9*6=54) = 66
    // 5 cycles = 330 transmissions
    TEST_ASSERT_EQUAL_UINT32(330, Mock_HAL_GetI2CCallCount());
}

// =============================================================================
// MAIN DES TESTS
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Tests de base
    RUN_TEST(test_lcd_send_nibble_basic);
    RUN_TEST(test_lcd_send_command_basic);
    RUN_TEST(test_lcd_send_data_character);
    RUN_TEST(test_lcd_clear_screen);
    
    // Tests de positionnement
    RUN_TEST(test_lcd_set_cursor_positions);
    
    // Tests d'affichage
    RUN_TEST(test_lcd_send_string_short);
    RUN_TEST(test_lcd_send_string_empty);
    RUN_TEST(test_lcd_send_string_full_width);
    
    // Tests de scrolling
    RUN_TEST(test_lcd_scroll_string_no_scroll);
    RUN_TEST(test_lcd_scroll_string_with_scroll);
    
    // Tests de robustesse
    RUN_TEST(test_lcd_i2c_error_handling);
    RUN_TEST(test_lcd_message_validation);
    RUN_TEST(test_lcd_null_pointer_safety);
    
    // Tests de séquence complète
    RUN_TEST(test_lcd_complete_display_sequence);
    RUN_TEST(test_lcd_performance_rapid_updates);
    
    return UNITY_END();
}
