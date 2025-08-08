#include "global.h"

volatile MachineState machine_interaction = IDLE;
volatile char keypad_choice[3] = "";
volatile uint8_t client_order = 0;

volatile char* lcd_display = "Pret";


