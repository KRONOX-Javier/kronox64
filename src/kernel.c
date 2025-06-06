// kernel.c - Core of KRONOX64 microkernel
// Freestanding, ultra-low level, x86_64, no stdlib

#include "include/io.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile unsigned short*) 0xB8000)

static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char text_color = 0x0F; // Light gray on black

// Set VGA text color (FG | BG << 4)
void set_color(unsigned char fg, unsigned char bg) {
    text_color = (bg << 4) | (fg & 0x0F);
}

// Clear entire screen
void clear_screen() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = (text_color << 8) | ' ';
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

// Scroll screen up by one line
void scroll() {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (text_color << 8) | ' ';
    }
    cursor_y--;
}

// Print a single character at the current cursor position
void put_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = (text_color << 8) | c;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }
}

// Print a null-terminated string
void print(const char* str) {
    while (*str) {
        put_char(*str++);
    }
}

// Print a number in hexadecimal (e.g., 0x1234ABCD)
void print_hex(unsigned long val) {
    const char* hex_chars = "0123456789ABCDEF";
    print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char digit = hex_chars[(val >> i) & 0xF];
        put_char(digit);
    }
}

// Halt the CPU forever
void panic(const char* msg) {
    set_color(15, 4); // white on red
    print("\n[ PANIC ] ");
    print(msg);
    print("\nSystem halted.\n");
    while (1) __asm__ volatile ("hlt");
}

// Entry point called from boot.s or Rust
void kernel_main(void) {
    set_color(15, 0);
    clear_screen();
    print(":: KRONOX64 microkernel boot sequence ::\n\n");

    gdt_install();
    idt_install();
    keyboard_init();

    print("System initialized successfully.\n");
    print("Type on the keyboard. Press ESC to halt.\n");

    while (1) {
        if (keyboard_has_char()) {
            char c = keyboard_read_char();

            if (c == 27) {
                panic("Escape key pressed");
            } else if (c) {
                put_char(c);
            }
        }
        __asm__ volatile ("hlt");
    }
}

