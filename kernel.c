#include <stdint.h>
#include "vga.h"
#include "idt.h"
#include "isr.h"
#include "shell.h"
#include "memory.h"
#include "multiboot.h"
#include "pmm.h"
#include "serial.h"
#include "paging.h"
#include "heap.h"
#include "timer.h"
#include "task.h"

void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
    serial_init();          // before anything prints: VGA output mirrors here
    clear_screen();
    print("JarvisOS 1.0.0 booting...\n");

    if (magic != MULTIBOOT_MAGIC) {
        print("FATAL: not booted by a Multiboot loader.\n");
        while (1) { __asm__("hlt"); }
    }

    memory_init(mbi);
    print("Memory map parsed: ");
    print_int(mem_usable_kb() / 1024);
    print(" MB usable RAM detected.\n");

    pmm_init();
    print("Physical allocator online: ");
    print_int(pmm_free_frames());
    print(" free frames (");
    print_int(pmm_free_frames() * 4 / 1024);
    print(" MB).\n");

    paging_init();
    print("Paging enabled: identity map 0 - ");
    print_hex(paging_identity_end());
    print(" (");
    print_int(paging_table_count());
    print(" page tables).\n");

    heap_init();
    print("Kernel heap ready at ");
    print_hex(HEAP_START);
    print(" (");
    print_int(heap_size() / 1024);
    print(" KB mapped).\n");

    idt_install();
    isr_install();

    scheduler_init();
    timer_init();
    print("Scheduler armed, PIT at ");
    print_int(TIMER_HZ);
    print(" Hz (");
    print_int(1000 / TIMER_HZ);
    print(" ms per tick).\n");

    __asm__ volatile ("sti");
    print("Interrupts online. Keyboard");
    print(serial_is_ready() ? " and COM1 serial ready.\n" : " ready (no serial port).\n");
    print("Type 'help' for commands.\n");

    shell_init();

    // Task 0 from here on: drain console input, then idle until the next
    // interrupt. The scheduler preempts this loop like any other task.
    while (1) {
        shell_poll();
        __asm__ volatile ("hlt");
    }
}
