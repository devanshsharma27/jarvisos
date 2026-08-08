#include "task.h"
#include "heap.h"
#include "vga.h"
#include "util.h"

// Round-robin preemptive scheduling. Every task owns a kernel stack taken
// from the heap; switching means swapping stack pointers, so a task's whole
// context lives on its own stack (see switch.asm).

extern void context_switch(uint32_t* save_esp, uint32_t new_esp);

static task_t   tasks[MAX_TASKS];
static int      current      = 0;
static int      enabled      = 0;
static uint32_t switches     = 0;
static uint32_t quantum_left = TASK_QUANTUM;

static void set_name(char* dest, const char* src) {
    int i = 0;
    while (src[i] && i < 15) { dest[i] = src[i]; i++; }
    dest[i] = '\0';
}

void scheduler_init(void) {
    for (int i = 0; i < MAX_TASKS; i++)
        tasks[i].state = TASK_UNUSED;

    // Task 0 is the code already running. It has no crafted stack: the first
    // switch away from it saves its real context wherever it happens to be.
    tasks[0].id         = 0;
    tasks[0].state      = TASK_RUNNING;
    tasks[0].stack_base = 0;
    tasks[0].esp        = 0;
    tasks[0].slices     = 1;
    tasks[0].counter    = 0;
    set_name(tasks[0].name, "kernel");

    current = 0;
    enabled = 1;
}

int task_create(const char* name, void (*entry)(void)) {
    int slot = -1;
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) { slot = i; break; }
    }
    if (slot < 0) return -1;

    uint8_t* stack = (uint8_t*) kmalloc(TASK_STACK_SIZE);
    if (!stack) return -1;

    // Build a stack that looks exactly like one context_switch just saved,
    // so the first switch into this task "returns" into entry().
    uint32_t* sp = (uint32_t*) (stack + TASK_STACK_SIZE);
    *--sp = (uint32_t) task_exit;   // where entry() returns to
    *--sp = (uint32_t) entry;       // the "ret" target in context_switch
    for (int i = 0; i < 8; i++)
        *--sp = 0;                  // the eight registers popa restores
    *--sp = 0x202;                  // eflags with IF set: it starts preemptible

    tasks[slot].esp        = (uint32_t) sp;
    tasks[slot].stack_base = (uint32_t) stack;
    tasks[slot].id         = (uint32_t) slot;
    tasks[slot].state      = TASK_READY;
    tasks[slot].slices     = 0;
    tasks[slot].counter    = 0;
    set_name(tasks[slot].name, name);

    return slot;
}

// A dead task's stack can be returned to the heap once we are no longer
// running on it -- that is, once it is not the current task.
static void reap(void) {
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_DEAD && i != current) {
            if (tasks[i].stack_base) kfree((void*) tasks[i].stack_base);
            tasks[i].stack_base = 0;
            tasks[i].state      = TASK_UNUSED;
        }
    }
}

static int pick_next(void) {
    for (int step = 1; step <= MAX_TASKS; step++) {
        int i = (current + step) % MAX_TASKS;
        if (tasks[i].state == TASK_READY || tasks[i].state == TASK_RUNNING)
            return i;
    }
    return -1;
}

// The heart of it: called from the timer interrupt, with the PIC already
// acknowledged so the next tick still arrives after we change stacks.
static void switch_to(int next) {
    if (next < 0 || next == current) return;

    int prev = current;
    if (tasks[prev].state == TASK_RUNNING) tasks[prev].state = TASK_READY;

    current = next;
    tasks[current].state = TASK_RUNNING;
    tasks[current].slices++;
    switches++;
    quantum_left = TASK_QUANTUM;

    context_switch(&tasks[prev].esp, tasks[current].esp);
    // Execution resumes here whenever somebody switches back to `prev`.
}

void scheduler_tick(void) {
    if (!enabled) return;

    reap();
    if (quantum_left > 1) { quantum_left--; return; }

    switch_to(pick_next());
}

void task_yield(void) {
    if (!enabled) return;
    switch_to(pick_next());
}

void task_exit(void) {
    tasks[current].state = TASK_DEAD;

    // Wait to be scheduled away; pick_next() skips dead tasks, so this task
    // is left behind on the next tick and reaped from another stack.
    __asm__ volatile ("sti");
    while (1) __asm__ volatile ("hlt");
}

int task_kill(uint32_t id) {
    if (id == 0 || id >= MAX_TASKS) return 0;          // task 0 is the kernel
    if (tasks[id].state == TASK_UNUSED) return 0;

    tasks[id].state = TASK_DEAD;
    return 1;
}

// The demo workload: spin, counting. `ps` shows the counters pulling apart,
// which is the visible proof that preemption is really happening.
static void counter_task(void) {
    int me = current;
    while (1)
        tasks[me].counter++;
}

int task_spawn_counter(const char* name) {
    return task_create(name, counter_task);
}

void scheduler_enable(int on)  { enabled = on; }
int  scheduler_is_enabled(void) { return enabled; }

task_t*  task_get(int index)    { return (index >= 0 && index < MAX_TASKS) ? &tasks[index] : 0; }
uint32_t task_current_id(void)  { return tasks[current].id; }
uint32_t task_switches(void)    { return switches; }

int task_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_TASKS; i++)
        if (tasks[i].state != TASK_UNUSED) n++;
    return n;
}
