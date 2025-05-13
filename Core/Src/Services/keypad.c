#include "keypad.h"
#include "global.h"
#include "string.h"

// Définition des ports et broches pour les lignes et colonnes
static GPIO_TypeDef* rowPorts[KEYPAD_ROWS] = {GPIOA, GPIOA, GPIOB, GPIOC};
static uint16_t rowPins[KEYPAD_ROWS] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_7};

static GPIO_TypeDef* colPorts[KEYPAD_COLS] = {GPIOA, GPIOB, GPIOB};
static uint16_t colPins[KEYPAD_COLS] = {GPIO_PIN_10, GPIO_PIN_4, GPIO_PIN_5};

static char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

volatile KeypadState keypad_interaction = IDLE;
volatile char keypad_choice[3] = "";
volatile uint8_t client_order = 0;

void Keypad_Init(void) {
    // À faire si besoin : GPIO déjà initialisés via CubeMX
    // On pourrait forcer les rows à HIGH ici si nécessaire
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_SET);
    }
}

static void setAllRowsHigh(void) {
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_SET);
    }
}

char Keypad_Scan(void) {
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        setAllRowsHigh();
        HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_RESET);
        HAL_Delay(1); // petit délai pour stabiliser

        for (int col = 0; col < KEYPAD_COLS; col++) {
            if (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_RESET) {
                while (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_RESET);
                return keymap[row][col];
            }
        }
    }
    return 0;
}

void StartTaskKeypad(void *argument) {
    printf("\r\nKeypad Task started\r\n");
    char key;
    size_t choice_length = 0;
    
    Keypad_Init();
    for (;;) {
        key = Keypad_Scan();
        if(key) {
            printf("Key pressed: %c\r\n", key);
            printf("Keypad interaction state: %d\r\n", keypad_interaction);
            switch (keypad_interaction){
                case IDLE:
                case ORDERING:
                    if(key == '*' && keypad_interaction == ORDERING) {
                        printf("Annuler\r\n");
                        keypad_choice[0] = '\0'; // Réinitialiser le choix
                        keypad_interaction = IDLE;
                    } else if (key != '*' && key != '#') {
                        printf("add number\r\n");
                        choice_length = strlen((const char *)keypad_choice);
                        keypad_choice[choice_length] = key;
                        keypad_choice[choice_length + 1] = '\0'; // Null-terminate the string
                        keypad_interaction = ORDERING;
                        if (strlen((const char *)keypad_choice) == 2) {
                            printf("Sending command to NFC reader\r\n");
                            client_order = (keypad_choice[0] - '0') * 10 + (keypad_choice[1] - '0');
                            printf("Client order: %d\r\n", client_order);
                            keypad_choice[0] = '\0';
                            keypad_interaction = PAYING;
                        }
                        osDelay(200); // délai pour éviter rebonds multiples
                    }
                    break;
                case PAYING:
                    if (key == '*') {
                        printf("Paiement annulé\r\n");
                        client_order = 0;
                        keypad_interaction = IDLE;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}
