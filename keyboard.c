#include <stdint.h>
#include "io.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"

static int shift_down = 0;

static const char normal_map[] = {
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0
};

static const char shift_map[] = {
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0
};

void keyboard_handler(void) {
    uint8_t sc = inb(0x60);

    // Shift press/release (left shift 0x2A, right shift 0x36)
    if (sc == 0x2A || sc == 0x36) { shift_down = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift_down = 0; return; }

    if (sc & 0x80) return;            // other key releases: ignore

    // Backspace (scancode 0x0E)
    if (sc == 0x0E) {
        shell_push_char('\b');
        return;
    }

    char c = 0;
    if (sc < sizeof(normal_map))
        c = shift_down ? shift_map[sc] : normal_map[sc];
    if (c == 0) return;

    // Queue it for the kernel task; the line editor runs outside interrupts.
    shell_push_char(c);
}
