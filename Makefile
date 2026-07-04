kkkkkkCC = gcc
LD = ld
CFLAGS = -m32 -ffreestanding -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o kernel.o vga.o idt.o idt_load.o isr.o isr_asm.o keyboard.o shell.o util.o

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

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.bin $(OBJS)

run: kernel.bin
	qemu-system-i386 -kernel kernel.bin

clean:
	rm -f *.o *.bin

.PHONY: all run clean
