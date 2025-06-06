// keyboard.c - Advanced PS/2 keyboard driver for KRONOX64 (bare-metal)
// No dependencies on libc or any OS standard libraries

#include "keyboard.h"
#include "io.h"

#define KB_DATA_PORT  0x60
#define KB_STATUS_PORT 0x64

#define BUFFER_SIZE 256

static char key_buffer[BUFFER_SIZE];
static unsigned int buf_head = 0;
static unsigned int buf_tail = 0;

static int shift = 0;
static int caps_lock = 0;
static int extended = 0;

// Simple scancode table (set 1)
static const char normal_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const char shift_map[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Push char into circular buffer
static void push_char(char c) {
    unsigned int next = (buf_head + 1) % BUFFER_SIZE;
    if (next != buf_tail) {
        key_buffer[buf_head] = c;
        buf_head = next;
    }
}

// Get char from buffer (non-blocking)
char keyboard_read_char() {
    if (buf_tail == buf_head) return 0;
    char c = key_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % BUFFER_SIZE;
    return c;
}

// Check if buffer has a char
int keyboard_has_char() {
    return buf_tail != buf_head;
}

// Main IRQ1 handler logic
void keyboard_handle_irq() {
    unsigned char status = inb(KB_STATUS_PORT);
    if (!(status & 0x01)) return;

    unsigned char sc = inb(KB_DATA_PORT);

    // Extended key?
    if (sc == 0xE0) {
        extended = 1;
        return;
    }

    // Key released?
    int released = sc & 0x80;
    sc = sc & 0x7F;

    // Handle modifier keys
    switch (sc) {
        case 0x2A: case 0x36: // LSHIFT, RSHIFT
            shift = !released;
            return;
        case 0x3A: // CAPSLOCK
            if (!released) caps_lock = !caps_lock;
            return;
    }

    if (released) return;

    char c = 0;
    if (shift ^ caps_lock)
        c = shift_map[sc];
    else
        c = normal_map[sc];

    if (c) push_char(c);

    extended = 0;
}

