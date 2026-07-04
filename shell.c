#include "shell.h"
#include "vga.h"
#include "util.h"
#include "io.h"

#define MAX_CMDS 32

static void shell_prompt(void) {
    print("jarvis> ");
}

// ---- Commands ----

static void cmd_help(const char* args) {
    (void)args;
    print("JarvisOS commands:\n");
    print("  help     - show this list\n");
    print("  clear    - clear the screen\n");
    print("  echo X   - print X back\n");
    print("  about    - about JarvisOS\n");
    print("  version  - kernel version\n");
    print("  calc     - calc 2 + 3  (also - * /)\n");
    print("  banner   - show the banner\n");
    print("  reboot   - restart the machine\n");
    print("  halt     - stop the CPU\n");
}

static void cmd_about(const char* args) {
    (void)args;
    print("JarvisOS - a hobby operating system built from scratch in C & asm.\n");
    print("Interrupt-driven keyboard, VGA driver, and now a shell.\n");
}

static void cmd_version(const char* args) {
    (void)args;
    print("JarvisOS 0.2.0 (32-bit x86)\n");
}

static void cmd_echo(const char* args) {
    print(args);
    print("\n");
}

static void cmd_banner(const char* args) {
    (void)args;
    print("     _                  _      ___  ____  \n");
    print("    | | __ _ _ ____   _(_)___ / _ \\/ ___| \n");
    print(" _  | |/ _` | '__\\ \\ / / / __| | | \\___ \\ \n");
    print("| |_| | (_| | |   \\ V /| \\__ \\ |_| |___) |\n");
    print(" \\___/ \\__,_|_|    \\_/ |_|___/\\___/|____/ \n");
}

static void cmd_calc(const char* args) {
    // Expected: "N op M" e.g. "12 + 5"
    int a = atoi(args);
    // advance past first number
    while (*args == ' ') args++;
    if (*args == '-') args++;
    while (*args >= '0' && *args <= '9') args++;
    while (*args == ' ') args++;
    char op = *args;
    if (op) args++;
    while (*args == ' ') args++;
    int b = atoi(args);

    int result = 0;
    if      (op == '+') result = a + b;
    else if (op == '-') result = a - b;
    else if (op == '*') result = a * b;
    else if (op == '/') {
        if (b == 0) { print("error: divide by zero\n"); return; }
        result = a / b;
    } else {
        print("usage: calc 2 + 3\n");
        return;
    }
    print_int(result);
    print("\n");
}

static void cmd_reboot(const char* args) {
    (void)args;
    print("Rebooting...\n");
    // Pulse the CPU reset line via the keyboard controller
    outb(0x64, 0xFE);
}

static void cmd_halt(const char* args) {
    (void)args;
    print("System halted. You can close QEMU.\n");
    __asm__ volatile ("cli; hlt");
}

static void cmd_clear(const char* args) {
    (void)args;
    clear_screen();
}

// ---- Dispatch ----

void shell_execute(const char* line) {
    // Skip empty lines
    if (line[0] == '\0') { shell_prompt(); return; }

    // Split "command args..." at the first space
    // Find the command word length
    int i = 0;
    char cmd[32];
    while (line[i] && line[i] != ' ' && i < 31) { cmd[i] = line[i]; i++; }
    cmd[i] = '\0';
    const char* args = line[i] ? line + i + 1 : line + i;

    if      (strcmp(cmd, "help") == 0)    cmd_help(args);
    else if (strcmp(cmd, "clear") == 0)   cmd_clear(args);
    else if (strcmp(cmd, "echo") == 0)    cmd_echo(args);
    else if (strcmp(cmd, "about") == 0)   cmd_about(args);
    else if (strcmp(cmd, "version") == 0) cmd_version(args);
    else if (strcmp(cmd, "calc") == 0)    cmd_calc(args);
    else if (strcmp(cmd, "banner") == 0)  cmd_banner(args);
    else if (strcmp(cmd, "reboot") == 0)  cmd_reboot(args);
    else if (strcmp(cmd, "halt") == 0)    cmd_halt(args);
    else {
        print("unknown command: ");
        print(cmd);
        print("  (try 'help')\n");
    }

    shell_prompt();
}

void shell_init(void) {
    print("\n");
    shell_prompt();
}

