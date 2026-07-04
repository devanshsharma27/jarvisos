#include <stdint.h>
#include "io.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"

#define BUF_SIZE 256

static char line_buf[BUF_SIZE];
static int  line_len = 0;
static int  shift_down = 0;

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
        if (line_len > 0) {
            line_len--;
            backspace();              // visual erase (added to vga)
        }
        return;
    }

    char c = 0;
    if (sc < sizeof(normal_map))
        c = shift_down ? shift_map[sc] : normal_map[sc];
    if (c == 0) return;

    if (c == '\n') {
        putchar('\n');
        line_buf[line_len] = '\0';
        line_len = 0;
        shell_execute(line_buf);      // hand the finished line to the shell
        return;
    }

    if (line_len < BUF_SIZE - 1) {
        line_buf[line_len++] = c;
        putchar(c);                   // echo as you type
    }
}
