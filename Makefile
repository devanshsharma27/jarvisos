CC = gcc
LD = ld
CFLAGS = -m32 -ffreestanding -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o kernel.o vga.o idt.o idt_load.o isr.o isr_asm.o keyboard.o shell.o util.o \
       memory.o pmm.o serial.o paging.o heap.o timer.o task.o switch.o

all: kernel.bin

boot.o: boot.asm
	nasm -f elf32 boot.asm -o boot.o

idt_load.o: idt_load.asm
	nasm -f elf32 idt_load.asm -o idt_load.o

isr_asm.o: isr.asm
	nasm -f elf32 isr.asm -o isr_asm.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

vga.o: vga.c
	$(CC) $(CFLAGS) -c vga.c -o vga.o

idt.o: idt.c
	$(CC) $(CFLAGS) -c idt.c -o idt.o

isr.o: isr.c
	$(CC) $(CFLAGS) -c isr.c -o isr.o

keyboard.o: keyboard.c
	$(CC) $(CFLAGS) -c keyboard.c -o keyboard.o

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c -o shell.o

util.o: util.c
	$(CC) $(CFLAGS) -c util.c -o util.o

memory.o: memory.c
	$(CC) $(CFLAGS) -c memory.c -o memory.o

pmm.o: pmm.c
	$(CC) $(CFLAGS) -c pmm.c -o pmm.o

serial.o: serial.c
	$(CC) $(CFLAGS) -c serial.c -o serial.o

paging.o: paging.c
	$(CC) $(CFLAGS) -c paging.c -o paging.o

heap.o: heap.c
	$(CC) $(CFLAGS) -c heap.c -o heap.o

timer.o: timer.c
	$(CC) $(CFLAGS) -c timer.c -o timer.o

task.o: task.c
	$(CC) $(CFLAGS) -c task.c -o task.o

switch.o: switch.asm
	nasm -f elf32 switch.asm -o switch.o

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.bin $(OBJS)

run: kernel.bin
	qemu-system-i386 -kernel kernel.bin

clean:
	rm -f *.o *.bin

.PHONY: all run clean
