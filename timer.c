#include "timer.h"
#include "io.h"
#include "task.h"

// The 8253/8254 programmable interval timer runs off a 1.193182 MHz crystal.
// Dividing that by our divisor gives the interrupt rate on IRQ0.
#define PIT_FREQUENCY 1193182
#define PIT_CHANNEL0  0x40
#define PIT_COMMAND   0x43

static volatile uint32_t ticks = 0;

void timer_init(void) {
    uint32_t divisor = PIT_FREQUENCY / TIMER_HZ;

    // 0x36: channel 0, send low byte then high byte, square wave, binary.
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t) (divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t) ((divisor >> 8) & 0xFF));
}

void timer_tick(void) {
    ticks++;
    scheduler_tick();       // may switch us to a different task
}

uint32_t timer_ticks(void)   { return ticks; }
uint32_t timer_seconds(void) { return ticks / TIMER_HZ; }

void timer_sleep(uint32_t ms) {
    uint32_t target = ticks + (ms * TIMER_HZ) / 1000;
    while (ticks < target)
        __asm__ volatile ("hlt");   // wake on the next interrupt
}
