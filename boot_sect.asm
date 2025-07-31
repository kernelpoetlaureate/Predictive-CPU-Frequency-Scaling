; boot_sect.asm - Minimal boot sector that jumps to 32-bit code at 0x7C00
BITS 16
ORG 0x7C00

    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Set up GDT for protected mode
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

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

[BITS 32]
pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    call main32
    jmp $

; Pad to 510 bytes, then boot signature
TIMES 510-($-$$) db 0
    dw 0xAA55
