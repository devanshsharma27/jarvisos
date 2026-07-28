#include "shell.h"
#include "vga.h"
#include "util.h"
#include "io.h"
#include "memory.h"
#include "pmm.h"

static void shell_prompt(void) {
    print("jarvis> ");
}

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
    print("  meminfo  - memory statistics\n");
    print("  memmap   - physical memory regions\n");
    print("  frames   - frame allocator status\n");
    print("  alloc    - allocate one 4KB frame\n");
    print("  free A   - free the frame at address A (hex, e.g. 0x200000)\n");
    print("  reboot   - restart the machine\n");
    print("  halt     - stop the CPU\n");
}

static void cmd_about(const char* args) {
    (void)args;
    print("JarvisOS - a hobby operating system built from scratch in C & asm.\n");
    print("Drivers, interrupts, shell, and a physical memory manager.\n");
}

static void cmd_version(const char* args) {
    (void)args;
    print("JarvisOS 0.4.0 (32-bit x86)\n");
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
    int a = atoi(args);
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

static void cmd_meminfo(const char* args) {
    (void)args;
    print("Total reported : ");
    print_int(mem_total_kb());
    print(" KB\n");
    print("Usable RAM     : ");
    print_int(mem_usable_kb());
    print(" KB (");
    print_int(mem_usable_kb() / 1024);
    print(" MB)\n");
    print("Highest address: ");
    print_hex(mem_highest_addr());
    print("\n");
    print("Free frames    : ");
    print_int(pmm_free_frames());
    print(" of ");
    print_int(pmm_total_frames());
    print("\n");
}

static void cmd_memmap(const char* args) {
    (void)args;
    memory_print_map();
}

static void cmd_frames(const char* args) {
    (void)args;
    print("Frame size   : 4096 bytes\n");
    print("Total frames : ");
    print_int(pmm_total_frames());
    print("\n");
    print("Used frames  : ");
    print_int(pmm_used_frames());
    print(" (");
    print_int(pmm_used_frames() * 4 / 1024);
    print(" MB)\n");
    print("Free frames  : ");
    print_int(pmm_free_frames());
    print(" (");
    print_int(pmm_free_frames() * 4 / 1024);
    print(" MB)\n");
    print("Bitmap at    : ");
    print_hex(pmm_bitmap_addr());
    print("  size ");
    print_int(pmm_bitmap_size());
    print(" bytes\n");
}

static void cmd_alloc(const char* args) {
    (void)args;
    uint32_t addr = pmm_alloc_frame();
    if (addr == 0) {
        print("out of memory!\n");
        return;
    }
    print("allocated frame at ");
    print_hex(addr);
    print("   (free now: ");
    print_int(pmm_free_frames());
    print(")\n");
}

static void cmd_free(const char* args) {
    uint32_t addr = (uint32_t) atoi_hex(args);
    if (addr == 0) {
        print("usage: free 0x200000\n");
        return;
    }
    pmm_free_frame(addr);
    print("freed frame at ");
    print_hex(addr);
    print("   (free now: ");
    print_int(pmm_free_frames());
    print(")\n");
}

static void cmd_reboot(const char* args) {
    (void)args;
    print("Rebooting...\n");
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

void shell_execute(const char* line) {
    if (line[0] == '\0') { shell_prompt(); return; }

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
    else if (strcmp(cmd, "meminfo") == 0) cmd_meminfo(args);
    else if (strcmp(cmd, "memmap") == 0)  cmd_memmap(args);
    else if (strcmp(cmd, "frames") == 0)  cmd_frames(args);
    else if (strcmp(cmd, "alloc") == 0)   cmd_alloc(args);
    else if (strcmp(cmd, "free") == 0)    cmd_free(args);
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
