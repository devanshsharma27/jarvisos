bits 32
global idt_load
idt_load:
    mov eax, [esp + 4]   ; get the argument (address of idtp)
    lidt [eax]           ; LIDT: load the IDT register
    ret
