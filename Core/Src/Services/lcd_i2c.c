#include "lcd_i2c.h"
#include "global.h"
#include <string.h>

#define LCD_ADDR         (0x27 << 1) // Adresse I2C du module (0x27 est classique)
#define LCD_BACKLIGHT    0x08
#define LCD_ENABLE       0x04
#define LCD_RW           0x02
#define LCD_RS           0x01

//extern I2C_HandleTypeDef hi2c1;
volatile char* lcd_display = NULL;

static void lcd_send_nibble(uint8_t nibble, uint8_t control) {
    uint8_t data = nibble | control | LCD_BACKLIGHT;

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, HAL_MAX_DELAY);
    data |= LCD_ENABLE;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, HAL_MAX_DELAY);
    osDelay(1);
    data &= ~LCD_ENABLE;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, HAL_MAX_DELAY);
    osDelay(1);
}

void lcd_send_command(uint8_t cmd) {
    lcd_send_nibble(cmd & 0xF0, 0);
    lcd_send_nibble((cmd << 4) & 0xF0, 0);
    osDelay(2);
}

void lcd_send_data(uint8_t data) {
    lcd_send_nibble(data & 0xF0, LCD_RS);
    lcd_send_nibble((data << 4) & 0xF0, LCD_RS);
    osDelay(2);
}

void lcd_clear(void) {
    lcd_send_command(0x01);
    osDelay(2);
}

void lcd_init(void) {
    osDelay(50); // Attente initiale après power-up

    // Init 4-bit mode sequence (voir datasheet)
    lcd_send_nibble(0x30, 0);
    osDelay(5);
    lcd_send_nibble(0x30, 0);
    osDelay(1);
    lcd_send_nibble(0x30, 0);
    osDelay(1);
    lcd_send_nibble(0x20, 0); // Passer en 4-bit
    osDelay(1);

    lcd_send_command(0x28); // 4-bit, 2 lignes, 5x8
    lcd_send_command(0x0C); // Display ON, cursor OFF
    lcd_send_command(0x06); // Entry mode
    lcd_clear();
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_send_command(0x80 | (col + row_offsets[row]));
}

void lcd_send_string(char *str) {
    while (*str) {
        lcd_send_data((uint8_t)*str++);
    }
}

void lcd_scroll_string(char *str, uint8_t row, uint16_t delay_ms) {
    char buffer[17]; // Pour un écran 16x2, 16 caractères + null terminator
    size_t len = strlen(str);

    if (len <= 16) {
        lcd_set_cursor(row, 0);
        lcd_send_string(str);
        return;
    }

    for (size_t i = 0; i <= len - 16; i++) {
        strncpy(buffer, &str[i], 16);
        buffer[16] = '\0';
        lcd_set_cursor(row, 0);
        lcd_send_string(buffer);
        osDelay(delay_ms);
    }
}

void lcd_backlight(uint8_t state) {
    uint8_t data = state ? LCD_BACKLIGHT : 0x00;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, HAL_MAX_DELAY);
}

void StartTaskLCD(void *argument) {
    printf("\r\nLCD Screen Task started\r\n");
    lcd_init();
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_send_string(lcd_display);

    KeypadState keypad_last_state = keypad_interaction;
    char order_number[10];

    for(;;) {
        if (keypad_interaction != keypad_last_state) {
            keypad_last_state = keypad_interaction;
            lcd_clear();
        }
        switch (keypad_interaction){
            case IDLE:
                lcd_display = "Choisissez une";
                lcd_set_cursor(1, 0);
                lcd_send_string("boisson");
                break;
            case ORDERING:
                lcd_display = "Choix de boisson";
                lcd_set_cursor(1, 0);
                lcd_send_string(keypad_choice);
                break;
            case PAYING:
                lcd_display = "Paiement en cours";
                sprintf(order_number, "%d", client_order);
                lcd_set_cursor(1, 0);
                lcd_send_string(order_number);
                break;
            case DELIVERING:
                lcd_display = "Distribution en cours";
                break;
            case SETTINGS:
                lcd_display = "Parametres";
                break;
        }
        lcd_set_cursor(0, 0);
        lcd_scroll_string(lcd_display, 0, 300);
    }
}

// void StartTaskLCD(void *argument) {
// 	printf("\r\nLCD Screen Task started\r\n");
//     lcd_init();
//     lcd_clear();
//     lcd_set_cursor(0, 0);
//     lcd_send_string("Init LCD OK");

//     int counter = 0;
//     char buffer[20];

//     for (;;) {
//         lcd_clear();
//         lcd_set_cursor(0, 0);
//         lcd_send_string("FreeRTOS LCD");
//         lcd_set_cursor(1, 0);
//         snprintf(buffer, sizeof(buffer), "Count: %d", counter++);
//         lcd_send_string(buffer);
//         osDelay(1000);
//     }
// }


// void I2C_Scanner(void) {
//     uint8_t i2c_address;
//     HAL_StatusTypeDef result;

//     for (i2c_address = 0x03; i2c_address < 0x78; i2c_address++) {
//         result = HAL_I2C_IsDeviceReady(&hi2c1, i2c_address << 1, 3, 10);
//         if (result == HAL_OK) {
//             printf("I2C device found at address 0x%02X\n", i2c_address);
//         }
//     }
// }
