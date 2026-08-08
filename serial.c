#include "serial.h"
#include "io.h"
#include "shell.h"

// COM1. The 16550 UART is eight consecutive I/O ports starting here.
#define COM1        0x3F8
#define REG_DATA    0       // read: received byte, write: byte to send
#define REG_IER     1       // interrupt enable (divisor high when DLAB=1)
#define REG_FCR     2       // FIFO control
#define REG_LCR     3       // line control (DLAB lives in bit 7)
#define REG_MCR     4       // modem control
#define REG_LSR     5       // line status

#define LSR_DATA_READY  0x01
#define LSR_THR_EMPTY   0x20

// A dead or absent UART never raises "transmitter empty", so every write is
// bounded: JarvisOS must still boot on a machine with no serial port.
#define TX_SPIN_LIMIT 100000

static int      ready    = 0;
static uint32_t rx_count = 0;
static uint32_t tx_count = 0;

void serial_init(void) {
    outb(COM1 + REG_IER, 0x00);   // interrupts off while we configure
    outb(COM1 + REG_LCR, 0x80);   // DLAB on: the next two ports are the divisor
    outb(COM1 + REG_DATA, 0x03);  // divisor low  = 3  -> 115200/3 = 38400 baud
    outb(COM1 + REG_IER, 0x00);   // divisor high = 0
    outb(COM1 + REG_LCR, 0x03);   // DLAB off, 8 bits, no parity, 1 stop bit
    outb(COM1 + REG_FCR, 0xC7);   // enable + clear FIFOs, trigger at 14 bytes
    outb(COM1 + REG_MCR, 0x0B);   // DTR + RTS + OUT2 (OUT2 gates the IRQ line)

    // Loopback test: echo 0xAE back to ourselves to prove a UART is there.
    outb(COM1 + REG_MCR, 0x1E);
    outb(COM1 + REG_DATA, 0xAE);
    ready = (inb(COM1 + REG_DATA) == 0xAE);

    outb(COM1 + REG_MCR, 0x0B);   // back to normal operation
    if (ready)
        outb(COM1 + REG_IER, 0x01);   // raise IRQ4 when a byte arrives
}

int serial_is_ready(void) { return ready; }

static void tx(char c) {
    uint32_t spins = 0;
    while (!(inb(COM1 + REG_LSR) & LSR_THR_EMPTY)) {
        if (++spins > TX_SPIN_LIMIT) return;
    }
    outb(COM1 + REG_DATA, (uint8_t) c);
    tx_count++;
}

void serial_write_char(char c) {
    if (!ready) return;
    if (c == '\n') tx('\r');      // terminals want CRLF
    tx(c);
}

void serial_write(const char* s) {
    while (*s) serial_write_char(*s++);
}

// IRQ4: one or more bytes arrived from the host. They go into the same input
// queue the keyboard feeds, so a serial client drives the shell identically.
void serial_handler(void) {
    while (inb(COM1 + REG_LSR) & LSR_DATA_READY) {
        uint8_t byte = inb(COM1 + REG_DATA);
        rx_count++;

        if (byte == '\r') byte = '\n';            // Enter over a terminal
        if (byte == 0x7F) byte = '\b';            // DEL -> backspace

        shell_push_char((char) byte);
    }
}

uint32_t serial_rx_count(void) { return rx_count; }
uint32_t serial_tx_count(void) { return tx_count; }
