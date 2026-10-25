BITS 16
ORG 0x7c00

_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti
    mov [boot_drive], dl
    
    mov ax, 0x13
    int 0x10

    mov si, dap
    mov dl, [boot_drive]
    mov ax, 0x4200
    int 0x13
    jc disk_error

    jmp 0x1000:0x0000

disk_error:
    mov ax, 0xb800
    mov es, ax
    xor di, di
    mov ax, 0x4f45
    stosw
    mov ax, 0x4f52
    stosw
.halt:
    hlt
    jmp .halt

boot_drive: db 0
align 4, db 0
dap:
    db 0x10, 0
    dw KERNEL_SECTORS
    dw 0x0000, 0x1000
    dq 1

times 510-($-$$) db 0
dw 0xaa55