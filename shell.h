#ifndef SHELL_H
#define SHELL_H

void shell_init(void);
void shell_execute(const char* line);

// Interrupt handlers push characters in from any console (PS/2 keyboard or
// serial port); the kernel task drains them with shell_poll(). Keeping the
// shell out of interrupt context means commands run with interrupts enabled
// and can be preempted like any other code.
void shell_push_char(char c);   // called from an interrupt handler
void shell_poll(void);          // called from the kernel main loop

// One character of input: echo, backspace, and dispatch of a finished line.
void shell_input_char(char c);

#endif

