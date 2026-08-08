#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS       8
#define TASK_STACK_SIZE 8192
#define TASK_QUANTUM    5       // ticks a task runs before it is preempted

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_DEAD
} task_state_t;

typedef struct task {
    uint32_t     esp;           // saved stack pointer (must stay first)
    uint32_t     stack_base;    // heap allocation backing the stack
    uint32_t     id;
    char         name[16];
    task_state_t state;
    uint32_t     slices;        // how many times it has been scheduled
    uint32_t     counter;       // work done, for the demo tasks
} task_t;

void scheduler_init(void);                  // adopt the running kernel as task 0
void scheduler_tick(void);                  // called once per timer interrupt
void scheduler_enable(int on);
int  scheduler_is_enabled(void);

int  task_create(const char* name, void (*entry)(void));
int  task_spawn_counter(const char* name);  // demo task: counts forever
int  task_kill(uint32_t id);
void task_exit(void);
void task_yield(void);

task_t*  task_get(int index);
int      task_count(void);
uint32_t task_current_id(void);
uint32_t task_switches(void);

#endif
