; boot.asm - Boot sector for QEMU, calls C kernel
BITS 16
ORG 0x7C00

; Set up stack
xor ax, ax
mov ss, ax
mov sp, 0x7C00

; Switch to 32-bit protected mode
cli
lgdt [gdt_desc]
mov eax, cr0
or eax, 1
mov cr0, eax
jmp 0x08:protected_mode

; GDT
align 8
gdt_start:
    dq 0x0000000000000000 ; null
    dq 0x00CF9A000000FFFF ; code
    dq 0x00CF92000000FFFF ; data
gdt_end:
gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; 32-bit code
[BITS 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    call kernel_main

    jmp $

; Kernel entry (C function)
extern kernel_main

times 510-($-$$) db 0
    dw 0xAA55
