#ifndef GLOBAL_H
#define GLOBAL_H

typedef enum {
    IDLE,
    ORDERING,
    PAYING,
    DELIVERING,
    SETTINGS,
} KeypadState;

extern volatile KeypadState keypad_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

extern volatile char* lcd_display;

#endif // GLOBAL_H