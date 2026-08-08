#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

void serial_init(void);
int  serial_is_ready(void);
void serial_write_char(char c);
void serial_write(const char* s);
void serial_handler(void);        // called from IRQ4

uint32_t serial_rx_count(void);
uint32_t serial_tx_count(void);

#endif
