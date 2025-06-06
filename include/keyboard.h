#ifndef KRONOX_KEYBOARD_H
#define KRONOX_KEYBOARD_H

void keyboard_handle_irq();  // called from IRQ1 handler
char keybaord_read_char();   // non-blocking read
int keyboard_has_char();     // true if char available
