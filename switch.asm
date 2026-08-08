bits 32

; void context_switch(uint32_t* save_esp, uint32_t new_esp)
;
; Saves the current task's registers on its own stack, records where that
; stack ended up, then swaps to the next task's stack and unwinds its saved
; registers. The "ret" at the end returns into whichever task now owns the
; stack -- which is how one function call enters a different task.
global context_switch
context_switch:
    pusha                   ; edi esi ebp esp ebx edx ecx eax  (32 bytes)
    pushfd                  ; eflags                           (4 bytes)

    ; Stack now: [esp]=eflags, [esp+4..35]=pusha, [esp+36]=return address,
    ;            [esp+40]=save_esp, [esp+44]=new_esp
    mov eax, [esp + 40]     ; where to store the outgoing stack pointer
    mov ecx, [esp + 44]     ; stack pointer of the task we are switching to
    mov [eax], esp          ; save the outgoing context

    mov esp, ecx            ; from here on we are on the other task's stack
    popfd                   ; restore its flags (this is what re-enables
                            ; interrupts for a freshly created task)
    popa                    ; restore its registers
    ret                     ; jump back into that task
