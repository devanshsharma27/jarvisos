#include "shell.h"
#include "vga.h"
#include "util.h"
#include "io.h"
#include "memory.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"
#include "timer.h"
#include "task.h"
#include "serial.h"

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
    print("  paging   - paging status\n");
    print("  virt A   - translate virtual address A to physical\n");
    print("  map A    - map a fresh frame at virtual address A\n");
    print("  unmap A  - unmap A and return its frame\n");
    print("  fault    - trigger a page fault on purpose (halts)\n");
    print("  heap     - kernel heap statistics and block list\n");
    print("  kmalloc N- allocate N bytes on the kernel heap\n");
    print("  kfree A  - free the heap pointer at address A (hex)\n");
    print("             (use the address kmalloc printed, e.g. 0xC0000010)\n");
    print("  uptime   - time since boot\n");
    print("  ps       - list tasks and context switches\n");
    print("  spawn N  - start a background task called N\n");
    print("  kill ID  - stop task ID\n");
    print("  sleep MS - sleep for MS milliseconds\n");
    print("  serial   - UART status and byte counts\n");
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
    print("JarvisOS 1.0.0 (32-bit x86)\n");
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

static void cmd_heap(const char* args) {
    (void)args;
    print("Heap base   : ");
    print_hex(HEAP_START);
    print("\nMapped      : ");
    print_int(heap_size() / 1024);
    print(" KB (break at ");
    print_hex(heap_break());
    print(")\nUsed        : ");
    print_int(heap_used());
    print(" bytes\nFree        : ");
    print_int(heap_free());
    print(" bytes\nBlocks      : ");
    print_int(heap_blocks());
    print(" total, ");
    print_int(heap_free_blocks());
    print(" free\n");
    heap_dump();
}

static void cmd_kmalloc(const char* args) {
    int size = atoi(args);
    if (size <= 0) { print("usage: kmalloc 128\n"); return; }

    void* p = kmalloc((uint32_t) size);
    if (!p) { print("kmalloc failed: heap exhausted\n"); return; }

    // Scribble on it so a broken mapping would show up immediately.
    memset(p, 0xAB, (uint32_t) size);

    print("kmalloc(");
    print_int(size);
    print(") = ");
    print_hex((uint32_t) p);
    print("   (physical ");
    print_hex(paging_translate((uint32_t) p));
    print(")\n");
}

static void cmd_kfree(const char* args) {
    uint32_t addr = atoi_hex(args);
    if (addr == 0) { print("usage: kfree 0xC000000C\n"); return; }

    if (!kfree((void*) addr)) return;   // kfree already said what was wrong

    print("freed ");
    print_hex(addr);
    print("   (heap free now: ");
    print_int(heap_free());
    print(" bytes)\n");
}

static void cmd_paging(const char* args) {
    (void)args;
    print("Paging      : ");
    print(paging_is_enabled() ? "ENABLED\n" : "disabled\n");
    print("Directory at: ");
    print_hex(paging_dir_phys());
    print("\nIdentity map: 0x00000000 - ");
    print_hex(paging_identity_end() - 1);
    print("\nPage tables : ");
    print_int(paging_table_count());
    print("\nMapped pages: ");
    print_int(paging_mapped_pages());
    print(" (");
    print_int(paging_mapped_pages() * 4 / 1024);
    print(" MB)\n");
}

static void cmd_virt(const char* args) {
    uint32_t virt = atoi_hex(args);
    uint32_t phys = paging_translate(virt);

    print_hex(virt);
    if (phys == PAGE_UNMAPPED) {
        print(" is not mapped\n");
        return;
    }
    print(" -> ");
    print_hex(phys);
    print(virt == phys ? "  (identity)\n" : "  (translated)\n");
}

// Allocate a fresh frame and drop it at a virtual address of your choosing,
// then prove the mapping works by writing through it and reading back.
static void cmd_map(const char* args) {
    uint32_t virt = atoi_hex(args) & ~(PAGE_SIZE - 1);
    if (virt == 0) {
        print("usage: map 0xD0000000\n");
        return;
    }
    if (paging_translate(virt) != PAGE_UNMAPPED) {
        print("that address is already mapped\n");
        return;
    }

    uint32_t frame = pmm_alloc_frame();
    if (frame == 0) { print("out of physical memory\n"); return; }

    if (!paging_map(virt, frame, PAGE_WRITE)) {
        pmm_free_frame(frame);
        print("mapping failed (no frame for a page table)\n");
        return;
    }

    volatile uint32_t* probe = (volatile uint32_t*) virt;
    *probe = 0x1234ABCD;
    print("mapped ");
    print_hex(virt);
    print(" -> ");
    print_hex(frame);
    print("\nwrote 0x1234ABCD, read back ");
    print_hex(*probe);
    print("\n");
}

static void cmd_unmap(const char* args) {
    uint32_t virt = atoi_hex(args) & ~(PAGE_SIZE - 1);
    uint32_t phys = paging_translate(virt);

    if (phys == PAGE_UNMAPPED) { print("not mapped\n"); return; }
    if (virt < paging_identity_end()) {
        print("refusing to unmap the identity region\n");
        return;
    }

    paging_unmap(virt);
    pmm_free_frame(phys & ~(PAGE_SIZE - 1));
    print("unmapped ");
    print_hex(virt);
    print(" and returned the frame\n");
}

static void cmd_fault(const char* args) {
    (void)args;
    print("Touching unmapped memory at 0xDEADB000 on purpose...\n");
    volatile uint32_t* bad = (volatile uint32_t*) 0xDEADB000;
    *bad = 1;
}

static void cmd_serial(const char* args) {
    (void)args;
    print("COM1 (0x3F8) : ");
    print(serial_is_ready() ? "present, 38400 8N1\n" : "not detected\n");
    print("Bytes sent   : ");
    print_int(serial_tx_count());
    print("\nBytes received: ");
    print_int(serial_rx_count());
    print("\nAll console output is mirrored here, and anything typed on the\n");
    print("serial line drives the same shell as the keyboard.\n");
}

static void cmd_uptime(const char* args) {
    (void)args;
    uint32_t secs = timer_seconds();

    print("Up ");
    print_int(secs / 60);
    print("m ");
    print_int(secs % 60);
    print("s  (");
    print_int(timer_ticks());
    print(" ticks at ");
    print_int(TIMER_HZ);
    print(" Hz)\n");
}

static void cmd_ps(const char* args) {
    (void)args;
    print("ID  NAME       STATE    SLICES   WORK\n");

    for (int i = 0; i < MAX_TASKS; i++) {
        task_t* t = task_get(i);
        if (!t || t->state == TASK_UNUSED) continue;

        print_int(t->id);
        print("   ");
        print(t->name);
        for (int pad = strlen(t->name); pad < 11; pad++) print(" ");

        if      (t->state == TASK_RUNNING) print("running  ");
        else if (t->state == TASK_READY)   print("ready    ");
        else                               print("dead     ");

        print_int(t->slices);
        print("      ");
        print_int(t->counter);
        print("\n");
    }

    print("Context switches: ");
    print_int(task_switches());
    print("   current task: ");
    print_int(task_current_id());
    print("\n");
}

static void cmd_spawn(const char* args) {
    const char* name = (*args) ? args : "worker";

    int id = task_spawn_counter(name);
    if (id < 0) {
        print("cannot spawn: no free task slot or heap space\n");
        return;
    }

    print("spawned task ");
    print_int(id);
    print(" ('");
    print(name);
    print("') with an ");
    print_int(TASK_STACK_SIZE / 1024);
    print(" KB stack from the heap\n");
}

static void cmd_kill(const char* args) {
    int id = atoi(args);
    if (!task_kill((uint32_t) id)) {
        print("no such task (and task 0 is the kernel itself)\n");
        return;
    }

    print("task ");
    print_int(id);
    print(" marked dead; its stack returns to the heap on the next tick\n");
}

static void cmd_sleep(const char* args) {
    int ms = atoi(args);
    if (ms <= 0) { print("usage: sleep 500   (milliseconds)\n"); return; }

    print("sleeping ");
    print_int(ms);
    print(" ms...\n");
    timer_sleep((uint32_t) ms);
    print("awake again at tick ");
    print_int(timer_ticks());
    print("\n");
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
    else if (strcmp(cmd, "heap") == 0)    cmd_heap(args);
    else if (strcmp(cmd, "kmalloc") == 0) cmd_kmalloc(args);
    else if (strcmp(cmd, "kfree") == 0)   cmd_kfree(args);
    else if (strcmp(cmd, "paging") == 0)  cmd_paging(args);
    else if (strcmp(cmd, "virt") == 0)    cmd_virt(args);
    else if (strcmp(cmd, "map") == 0)     cmd_map(args);
    else if (strcmp(cmd, "unmap") == 0)   cmd_unmap(args);
    else if (strcmp(cmd, "fault") == 0)   cmd_fault(args);
    else if (strcmp(cmd, "serial") == 0)  cmd_serial(args);
    else if (strcmp(cmd, "uptime") == 0)  cmd_uptime(args);
    else if (strcmp(cmd, "ps") == 0)      cmd_ps(args);
    else if (strcmp(cmd, "spawn") == 0)   cmd_spawn(args);
    else if (strcmp(cmd, "kill") == 0)    cmd_kill(args);
    else if (strcmp(cmd, "sleep") == 0)   cmd_sleep(args);
    else if (strcmp(cmd, "reboot") == 0)  cmd_reboot(args);
    else if (strcmp(cmd, "halt") == 0)    cmd_halt(args);
    else {
        print("unknown command: ");
        print(cmd);
        print("  (try 'help')\n");
    }

    shell_prompt();
}

// ---- line editor --------------------------------------------------
// Shared by every console: the PS/2 keyboard and the serial port both
// funnel single characters in here.

#define LINE_MAX  128
#define RING_SIZE 256

static char line_buf[LINE_MAX];
static int  line_len = 0;

// Single-producer (interrupt) / single-consumer (kernel task) ring buffer.
static volatile char     ring[RING_SIZE];
static volatile uint32_t ring_head = 0;   // written by the interrupt
static volatile uint32_t ring_tail = 0;   // written by the kernel task

void shell_push_char(char c) {
    uint32_t next = (ring_head + 1) % RING_SIZE;
    if (next == ring_tail) return;        // buffer full: drop the character
    ring[ring_head] = c;
    ring_head = next;
}

void shell_poll(void) {
    while (ring_tail != ring_head) {
        char c = ring[ring_tail];
        ring_tail = (ring_tail + 1) % RING_SIZE;
        shell_input_char(c);
    }
}

void shell_input_char(char c) {
    if (c == '\b') {
        if (line_len > 0) {
            line_len--;
            backspace();
        }
        return;
    }

    if (c == '\n') {
        putchar('\n');
        line_buf[line_len] = '\0';
        line_len = 0;
        shell_execute(line_buf);
        return;
    }

    if (c < ' ' || c > '~') return;         // ignore other control codes

    if (line_len < LINE_MAX - 1) {
        line_buf[line_len++] = c;
        putchar(c);                         // echo as you type
    }
}

void shell_init(void) {
    print("\n");
    shell_prompt();
}
