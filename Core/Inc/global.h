#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>

typedef enum {
    IDLE,
    ORDERING,
    PAYING,
    DELIVERING,
    SETTINGS,
} MachineState;

extern volatile MachineState machine_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

extern volatile char* lcd_display;

#endif // GLOBAL_H