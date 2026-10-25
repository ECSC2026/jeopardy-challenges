BITS 16
ORG 0

_start:
    cli
    mov ax, cs
    mov ds, ax
    mov es, ax

    mov ax, 0x7000
    mov ss, ax
    mov sp, 0xfffe

    mov ax, 0x5000
    mov es, ax
    mov ax, 0x8000
    mov gs, ax
    mov ax, 0x9000
    mov fs, ax

    cld
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

io_in8:

    mov dx, ax
    in al, dx
    xor ah, ah
    ret

io_out8:

    mov cl, dl
    mov dx, ax
    mov al, cl
    out dx, al
    ret

framebuffer_put:

    push di
    mov di, ax
    mov al, dl
    mov [es:di], al
    pop di
    ret

depth_get:

    push di
    mov di, ax
    xor ax, ax
    mov al, [gs:di]
    mov ah, [fs:di]
    pop di
    ret

depth_put:

    push di
    mov di, ax
    mov [gs:di], dl
    mov [fs:di], dh
    pop di
    ret

depth_clear:

    push es
    push di
    cld

    mov ax, gs
    mov es, ax
    xor di, di
    mov cx, 32000
    mov ax, 0ffffh
    rep stosw

    mov ax, fs
    mov es, ax
    xor di, di
    mov cx, 32000
    mov ax, 0ffffh
    rep stosw

    pop di
    pop es
    ret

present_frame:

    push ds
    push es
    push si
    push di
    cld

    mov ax, 5000h
    mov ds, ax
    mov ax, 0a000h
    mov es, ax
    xor si, si
    xor di, di
    mov cx, 32000
    rep movsw

    pop di
    pop si
    pop es
    pop ds
    ret

cpu_cli:
    cli
    ret

cpu_sti:
    sti
    ret

asset_read_u8:

    push bx
    mov bx, ax
    add bx, dx
    xor ax, ax
    mov al, [bx]
    pop bx
    ret

asset_read_u16:

    push bx
    mov bx, ax
    shl dx, 1
    add bx, dx
    mov ax, [bx]
    pop bx
    ret

div_u32_u16_sat:
    xchg dx, ax
    test cx, cx
    jz .saturated
    cmp dx, cx
    jae .saturated
    div cx
    ret
.saturated:
    xor ax, ax
    dec ax
    ret

screen_offset:

    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov cx, ax
    shl ax, 1
    shl ax, 1
    add ax, cx
    add ax, dx
    ret

unsigned_abs:
    mov cx, ax
    sar cx, 0xf
    add ax, cx
    xor ax, cx
    ret

divide_u16:
    test dx, dx
    jz .no_crash
    mov cx, dx
    xor dx, dx
    div cx
    ret
    .no_crash:
    xor ax, ax
    dec ax
    ret

divide_s16:
    test dx, dx
    jz .no_crash
    mov cx, dx
    cwd
    idiv cx
    xor dx, dx
    ret
    .no_crash:
    xor ax, ax
    ret

multiply_divide_u16:
    test cx, cx
    jz .no_crash
    mul dx
    cmp dx, cx
    jae .no_crash
    div cx
    ret
    .no_crash:
    xor ax, ax
    dec ax
    ret

multiply_divide_s16:
    test cx, cx
    jz .zero

    push bx

    mov bx, ax
    xor bx, dx
    xor bx, cx

    test ax, ax
    jns .a_pos
    neg ax
    .a_pos:

    test dx, dx
    jns .b_pos
    neg dx
    .b_pos:

    test cx, cx
    jns .divisor_pos
    neg cx
    .divisor_pos:
    mul dx

    cmp dx, cx
    jae .overflow

    div cx

    test bx, bx
    js .neg

    cmp ax, 0x7fff
    jbe .done
    mov ax, 0x7fff
    jmp .done
    .neg:
    cmp ax, 0x8000
    jae .neg_sat
    neg ax
    jmp .done
    .overflow:
    test bx, bx
    js .neg_sat
    mov ax, 0x7fff
    jmp .done
    .neg_sat:
    mov ax, 0x8000
    .done:
    pop bx
    ret
    .zero:
    xor ax, ax
    ret

divide_q3:
    test dx, dx
    jz .no_crash
    mov cx, dx
    cwd
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    idiv cx
    ret
    .no_crash:
    xor ax, ax
    ret

multiply_shift14:
    imul dx

    cmp dx, 0x2000
    jge .sat_pos

    cmp dx, 0xe000
    jl .sat_neg

    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1

    mov ax, dx
    ret
    .sat_pos:
    mov ax, 0x7fff
    ret
    .sat_neg:
    mov ax, 0x8000
    ret

saturated_add_s16:
    add ax, dx
    jno .done

    test dx, dx
    js .overflow_n

    mov ax, 0x7FFF
    ret
    .overflow_n:
    mov ax, 0x8000
    .done:
    ret

saturated_add_u16:
    add dx, ax
    mov ax, 0xffff
    jb .done
    mov ax, dx
    .done:
    ret

clamp_s16:
    cmp ax, cx
    jg .higher
    mov cx, ax
    .higher:
    cmp ax, dx
    jl .lower
    mov dx, cx
    .lower:
    mov ax, dx
    ret

camera_ray_component:
    push ax
    mov ax, dx
    mov dx, cx
    call multiply_shift14
    mov dx, ax
    pop ax
    call saturated_add_s16
    ret
    
    
add_player_x:
    add ax, word [player_x]
    ret

add_player_y:
    add ax, word [player_y]
    ret

sine_q14:
    push bx
    mov dl, ah
    mov bl, ah
    and bx, 0x003F
    test dl, 0x40
    jz .done
    neg bx
    add bx, 64
    .done:
    shl bx, 1
    mov ax, [sine_quarter_q14 + bx]
    test dl, 0x80
    jz .noneg
    neg ax
    .noneg:
    pop bx
    ret 

cosine_q14:
    add ax, 0x4000
    jmp sine_q14
    
map_cell:
	push si
	push di
	push bx
    mov bx, ax
	mov cx, 0x801
	bt cx, bx
	sbb si, si
	mov cx, 0x1001
	bt cx, dx
	sbb di, di
	or si, di
	mov cx, 0x108
	bt cx, bx
	sbb di, di
	mov cx, 0x3F8
	bt cx, dx
	sbb ax, ax
	and di, ax
	or si, di
	mov cx, 0x208
	bt cx, dx
	sbb di, di
	mov cx, 0x1F8
	bt cx, bx
	sbb ax, ax
	and di, ax
	or si, di
	mov ax, si
	and ax, 1
	pop bx 
	pop di
	pop si
	ret

wait_vertical_retrace:
    xor cx, cx
    .null_loop:
    inc cx
    jz .waited_null
    mov dx, 0x3da
    in al, dx
    test al, 0x8
    jnz .null_loop
    .waited_null:
    xor cx, cx
    .not_null_loop:
    inc cx
    jz .waited_not_null
    mov dx, 0x3da
    in al, dx
    test al, 0x8
    jz .not_null_loop
    .waited_not_null:
    ret
    
    
kbc_wait_write_ready:
    xor cx, cx
    .null_loop:
    inc cx
    jz .waited_null
    mov dx, 0x64
    in al, dx
    test al, 0x2
    jnz .null_loop
    xor ax, ax
    inc ax
    ret
    .waited_null:
    xor ax, ax
    ret

kbc_wait_for_byte:
    xor cx, cx
    .not_null_loop:
    inc cx
    jz .waited_non_null
    mov dx, 0x64
    in al, dx
    test al, 0x1
    jz .not_null_loop
    mov byte [kbc_status_byte], al
    mov dx, 0x60
    in al, dx
    mov byte [kbc_data_byte], al
    xor ax, ax
    inc ax
    ret 
    .waited_non_null:
    xor ax, ax
    ret 
    
kbc_flush:
    mov cx, 0xfbff ; count up to 0x1 0000 to overflow (this is 1024 iterations)
    .null_loop:
    inc cx
    jz .waited_non_null
    mov dx, 0x64
    in al, dx
    test al, 0x1
    jz .waited_non_null
    mov dx, 0x60
    in al, dx
    jmp .null_loop
    .waited_non_null:
    ret 

mouse_send_command:
    push di
    push si
    mov di, 3 ; 3 retries
    mov si, ax ; command argument
    .retry:
    call kbc_wait_write_ready
    test ax, ax
    jz .ret0
    

    mov al, 0xd4
    mov dx, 0x64
    out dx, al
    
    call kbc_wait_write_ready
    test ax, ax
    jz .ret0
    
    mov ax, si
    mov dx, 0x60
    out dx, al
    
    .loop:
    
    call kbc_wait_for_byte
    test ax, ax
    jz .ret0
    
    
    test byte [kbc_status_byte], 0x20
    jz .loop
    
    cmp byte [kbc_data_byte], 0xfa
    jz .ret1
    
    cmp byte [kbc_data_byte], 0xfe
    jnz .loop
    
    dec di
    jnz .retry

    .ret0:
    xor ax, ax
    pop si
    pop di 
    ret
    .ret1:
    xor ax, ax
    inc ax
    pop si
    pop di 
    ret

initialize_mouse:
    cli
    call kbc_flush
    
    call kbc_wait_write_ready
    test ax, ax
    jz .skip0
    mov dx, 0x64
    mov al, 0xa8
    out dx, al
    .skip0:
    
    call kbc_wait_write_ready
    test ax, ax
    jz .skip1
    mov dx, 0x64
    mov al, 0xae
    out dx, al
    .skip1:

    call kbc_wait_write_ready
    test ax, ax
    jz .skip2
    mov dx, 0x64
    mov al, 0x20
    out dx, al
    .skip2:
    
    call kbc_wait_for_byte
    test ax, ax
    jz .skip3
    
    call kbc_wait_write_ready
    test ax, ax
    jz .skip4
    mov dx, 0x64
    mov al, 0x60
    out dx, al
    .skip4:

    call kbc_wait_write_ready
    test ax, ax
    jz .skip3
    
    mov al, byte [kbc_data_byte]
    and al, 0xcc
    mov dx, 0x60
    out dx, al

    .skip3:
    
    call kbc_flush
    mov ax, 0xf6
    call mouse_send_command
    mov ax, 0xf4
    call mouse_send_command
    
    xor ax, ax 
    mov byte [mouse_phase], al
    mov byte [keyboard_extended], al
    mov word [pending_mouse_dx], ax
    sti
    ret

poll_input:
     .loop:
     mov dx, 0x64
     in al, dx
     test al, 1
     jz .done
     xchg al, ah
     mov dx, 0x60
     in al, dx
     test ah, 0x20
     jz .handle_keyboard
     xor ah, ah
     call handle_mouse_byte
     jmp .loop
     .handle_keyboard:
     xor ah, ah
     call handle_keyboard_byte
     jmp .loop
     .done:
     ret


handle_mouse_byte:
    mov cl, byte [mouse_phase]
    cmp cl, 0x1
    jz .opt1
    test cl, cl
    jne .opt2
    test al, 0x8
    jz .done
    
    .opt0:
    mov byte [mouse_packet_0], al
    mov byte [mouse_phase], 1
    ret
    
    .opt1:
    mov byte [mouse_packet_1], al
    mov byte [mouse_phase], 2
    ret
    
    .opt2:
    mov byte [mouse_phase], 0
    cmp byte [mouse_packet_0], 0x3f
    ja .done
    
    movsx dx, byte [mouse_packet_1]
    mov ax, word [pending_mouse_dx]
    call saturated_add_s16
    mov word [pending_mouse_dx], ax
    
    .done:
    ret
    
handle_keyboard_byte:
    cmp al, 0xE0
    jnz .skip0
    mov byte [keyboard_extended], 1
    ret
    .skip0:
    mov ah, al
    and ah, 0x7F
    xor cl, cl
    cmp byte [keyboard_extended], 0
    jne .keyboard_zero
    .keyboard_not_zero:
    cmp ah, 0x11
    jnz .not_forward0
    mov cl, 1
    jmp .done
    .not_forward0:
    cmp ah, 0x1F
    jnz .not_backwards0
    mov cl, 2
    jmp .done
    .not_backwards0:
    cmp ah, 0x1E
    jnz .not_left0
    mov cl, 4
    jmp .done
    .not_left0:
    cmp ah, 0x20
    jnz .not_right0
    mov cl, 8
    jmp .done
    .not_right0:
    jmp .done
    .keyboard_zero:
    cmp ah, 0x48
    jnz .not_forward1
    mov cl, 1
    jmp .done
    .not_forward1:
    cmp ah, 0x50
    jnz .not_backwards1
    mov cl, 2
    jmp .done
    .not_backwards1:
    cmp ah, 0x4B
    jnz .not_left1
    mov cl, 4
    jmp .done
    .not_left1:
    cmp ah, 0x4D
    jnz .not_right1
    mov cl, 8
    jmp .done
    .not_right1:
    .done:
    mov byte [keyboard_extended], 0
    test cl, cl
    jnz .not_zero
    ret
    .not_zero:
    mov ah, byte [held_keys]
    test al, 0x80
    jz .released_zero
    not cl
    and ah, cl
    mov byte [held_keys], ah
    ret
    .released_zero:
    or ah, cl
    mov byte [held_keys], ah
    ret

apply_mouse_rotation:
    mov ax, word [pending_mouse_dx]
    and word [pending_mouse_dx], 0
    shl ax, 6
    add word [view_angle], ax
    ret 

read_pit_counter:
    mov dx, 0x43
    xor al, al
    out dx, al
    mov dx, 0x40
    in al, dx
    mov ah, al
    in al, dx
    xchg al, ah
    ret

limit_frame_rate:
    call read_pit_counter
    mov bx, word [frame_start_pit]
    sub bx, ax
    cmp bx, 0x9b5d ; frame limiting to ~30fps
    ja .done
    call poll_input
    jmp limit_frame_rate
    .done:
    ret

initialize_palette:
    push bx
    mov dx, 0x3c8
    mov al, 0
    out dx, al
    
    mov dx, 0x3c9
    xor cx, cx
    
    .index_loop:
    cmp cx, 0x100
    jge .done

    xor bx, bx
    xor ax, ax
    
    cmp cx, 0xC8
    jne .not_c8
    mov bx, 0x3333
    mov ah, 0x33
    jmp .out_io
    .not_c8:
    
    cmp cx, 0xF
    jne .not_f
    mov bx, 0x3737
    mov ah, 0x37
    jmp .out_io
    .not_f:
    
    cmp cx, 0x2
    jne .not_2
    mov bx, 0x0707
    mov ah, 0x07
    jmp .out_io
    .not_2:
    
    cmp cx, 0x10
    jl .out_io
    cmp cx, 0x50
    jge .not_10_to_50
    mov al, cl
    sub al, 0x10
    shr al, 1
    mov ah, 0x23
    add ah, al
    mov bl, 0x8
    add bl, al
    mov bh, 0x2d
    sub bh, al
    jmp .out_io
    .not_10_to_50:
    cmp cx, 0x70
    jge .not_50_to_70
    mov al, cl
    sub al, 0x50
    mov ah, 0xC
    add ah, al
    mov bl, 0x3
    shr al, 2
    add bl, al
    mov bh, 0x2
    shr al, 1
    add bh, al
    jmp .out_io
    
    .not_50_to_70:
    cmp cx, 0x90
    jge .out_io
    
    mov al, cl
    sub al, 0x70
    add al, 0xC
    mov ah, al
    mov bl, al
    mov bh, al
    
    .out_io:
    mov al, ah ; red
    out dx, al
    mov al, bl ; green
    out dx, al
    mov al, bh ; blue
    out dx, al
    
    inc cx
    jmp .index_loop
    .done:
    pop bx
    ret

render_sky:
    push di
    push si
    push bx
    xor bx, bx ; x
    xor si, si ; y
    xor di, di ; offset
    .loop_y:
    cmp si, 0x64
    jge .done
    xor bx, bx
    .loop_x:
    cmp bx, 0x140
    jge .done_y
    mov dx, si
    shr dx, 1
    add dx, 0x10
    and dx, 0xff
    mov [es:di], dl
    inc di
    inc bx
    jmp .loop_x
    .done_y:
    inc si
    jmp .loop_y
    .done:
    pop bx
    pop si
    pop di
    ret

swap_vertices_01:
    mov ax, word [projected_x0]
    mov cx, word [projected_x1]
    mov word [projected_x0], cx
    mov word [projected_x1], ax
    mov ax, word [projected_y0]
    mov cx, word [projected_y1]
    mov word [projected_y0], cx
    mov word [projected_y1], ax
    mov ax, word [projected_z0]
    mov cx, word [projected_z1]
    mov word [projected_z0], cx
    mov word [projected_z1], ax
    ret


swap_vertices_12:
    mov ax, word [projected_x1]
    mov cx, word [projected_x2]
    mov word [projected_x1], cx
    mov word [projected_x2], ax
    mov ax, word [projected_y1]
    mov cx, word [projected_y2]
    mov word [projected_y1], cx
    mov word [projected_y2], ax
    mov ax, word [projected_z1]
    mov cx, word [projected_z2]
    mov word [projected_z1], cx
    mov word [projected_z2], ax
    ret
    

q3_to_integer:
    test ax, ax 
    js .negative
    shr ax, 3
    ret
    .negative:
    neg ax
    shr ax, 0x3
    neg ax 
    ret

edge_start_q3:
    push bx
    shl ax, 3 
    mov bx, ax
    mov ax, dx
    mov dx, cx
    xor cx, cx
    inc cx
    call multiply_divide_s16
    mov dx, ax
    mov ax, bx
    call saturated_add_s16
    pop bx
    ret 

edge_start_z:
    push bx
    mov bx, ax
    mov ax, dx
    mov dx, cx
    xor cx, cx
    inc cx
    call multiply_divide_s16
    mov dx, ax
    mov ax, bx
    call saturated_add_s16
    pop bx
    ret 

render_model:
    push si
    push di
    mov ax, word [view_angle]
    call sine_q14
    mov word [model_sin], ax 
    mov ax, word [view_angle]
    call cosine_q14
    mov word [model_cos], ax 
    xor si, si
    .triangle_loop:
    cmp si, 0x69e
    jge .done 
    test si, 0x1f
    jnz .no_poll
    call poll_input
    
    .no_poll:
    mov di, si
    add di, si
    mov ax, word [tri_p0+di]
    and ax, 0x3FF
    xor dx, dx
    call project_model_vertex
    mov ax, word [tri_p0+di]
    shr ax, 10
    mov cx, word [tri_p1+di]
    and cx, 0xF
    shl cx, 6
    or ax, cx 
    xor dx, dx
    inc dx
    call project_model_vertex
    mov ax, word [tri_p1+di]
    shr ax, 4
    and ax, 0x3FF
    xor dx, dx
    inc dx
    inc dx
    call project_model_vertex
    cmp word [projected_z0], 0
    jz .done_iter
    cmp word [projected_z1], 0
    jz .done_iter
    cmp word [projected_z2], 0
    jz .done_iter
    mov ax, word [tri_p1+di]
    test ax, 0x4000
    jz .color2
    mov ax, 0xc8
    call rasterize_triangle
    jmp .done_iter 
    .color2:
    xor ax, ax
    call rasterize_triangle
    .done_iter:
    inc si
    jmp .triangle_loop
    .done:
    pop di
    pop si
    ret

render_floor:
    push si
    push di
    push bx
    push bp
    mov ax, [view_angle]
    call sine_q14
    mov si, ax
    mov ax, [view_angle]
    call cosine_q14
    mov bx, ax
    mov di, 0x7d00
    mov ax, 0x64 ;y

    .y_loop:
    cmp ax, 0xc8
    jae .done
    test al, 7
    jnz .no_poll
    push ax
    call poll_input
    pop ax
    .no_poll:
    inc ax
    xor ah, ah
    push ax
    dec ax
    sub ax, 0x64
    jnz .distance_nonzero
    inc ax
    .distance_nonzero:

    mov cx, ax
    cmp cx, 6
    jbe .max_depth
    mov ax, 0x2000
    mov dx, 3
    div cx
    mov cx, ax
    jmp .depth_done

    .max_depth:
    mov cx, 0x7fff

    .depth_done:
    mov bp, 0xc0cd

    .ray_loop:
    mov ax, bx
    imul bp
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    mov ax, dx
    add ax, si
    imul cx
    cmp dx, 0x2000
    jge .world_x_sat_pos
    cmp dx, 0xe000
    jl .world_x_sat_neg
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    mov ax, dx
    jmp .world_x_done
    .world_x_sat_pos:
    mov ax, 0x7fff
    jmp .world_x_done
    .world_x_sat_neg:
    mov ax, 0x8000
    
    .world_x_done:
    add ax, [player_x]
    shr ax, 8
    and ax, 1
    push ax
    mov ax, si
    neg ax
    imul bp
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    mov ax, dx
    add ax, bx
    imul cx
    cmp dx, 0x2000
    jge .world_y_sat_pos
    cmp dx, 0xe000
    jl .world_y_sat_neg
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    mov ax, dx
    jmp .world_y_done
    .world_y_sat_pos:
    mov ax, 0x7fff
    jmp .world_y_done
    .world_y_sat_neg:
    mov ax, 0x8000
    .world_y_done:
    add ax, [player_y]

    shr ax, 8
    and ax, 1
    pop dx
    add ax, dx
    and ax, 1
    dec al
    and al, 0xd
    add al, 2
    mov ah, al
    mov [es:di], ax
    mov [es:di + 2], ax
    add di, 4

    pop ax

    add bp, 0x199
    add ah, 3
    cmp ah, 5
    jb .camera_no_carry
    inc bp
    sub ah, 5
    
    .camera_no_carry:
    cmp bp, 0x40cd
    je .row_done
    push ax
    jmp .ray_loop

    .row_done:
    xor ah, ah
    jmp .y_loop
    .done:
    pop bp
    pop bx
    pop di
    pop si
    ret
    
GLOBAL wall_pixel_color
wall_pixel_color:
    test dl, 0x10
    jz .skip_offset
    add ax, 0x10
    .skip_offset:
    and ax, 0x1f ;al=brick_x
    shr cx, 0x9
    cmp cx, 0x1f
    jbe .skip_darkness_fix
    mov cx, 0x1f
    .skip_darkness_fix:
    mov ch, 0x1f
    sub ch, cl ; ch=light
    
    xor ah, ah ;ah = mortar
    cmp al, 2
    jb .mortar_one
    mov dh, dl
    and dh, 0xf
    cmp dh, 2
    jl .mortar_one
    jmp .mortar_done
    .mortar_one:
    inc ah
    .mortar_done:
    
    cmp byte [ray_side], 0
    jz .skip_light_fix
    cmp ch, 4
    jbe .skip_light_fix
    sub ch, 4
    .skip_light_fix:
    
    cmp byte [ray_map_x], 5
    jnz .not_special
    cmp byte [ray_map_y], 3
    jnz .not_special
    test ah, ah
    mov ax, 0x70
    jz .special_not_mortar
    add ax, 0x10
    .special_not_mortar:
    shr ch, 1
    add al, ch
    ret
    .not_special:
    
    test ah, ah
    mov ax, 0x50
    jz .not_mortar
    add ax, 0x20
    .not_mortar:
    add al, ch
    ret

render_walls:
    push si
    push di
    push bx
    push bp

    mov ax, [view_angle]
    call sine_q14
    mov di, ax
    mov ax, [view_angle]
    call cosine_q14
    mov bx, ax
    mov bp, 0xc033 ;bp=camera_q14
    
    xor si, si ;si=x

    xor ax, ax
    push ax

    .x_loop:
    cmp si, 320
    jae .done

    test si, 31
    jnz .no_poll
    call poll_input
    .no_poll:
    
    mov ax, bx
    imul bp
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    mov ax, di
    add ax, dx

    push ax
    mov ax, di
    neg ax
    imul bp
    shl ax, 1
    rcl dx, 1
    shl ax, 1
    rcl dx, 1
    mov ax, bx
    add ax, dx
    mov dx, ax ;dx=ray_y
    pop ax ;ax=ray_x

    call cast_ray
    cmp byte [ray_hit], 0
    jz .no_hit

    push si ;x
    push di ;sin_view
    push bx ;cos_view
    push bp ;camera_q14
    
    mov cx, [ray_distance_q6]
    test cx, cx
    jnz .distance_nonzero
    inc cx
    .distance_nonzero:
    cmp cx, 6
    jbe .wall_height_sat
    mov ax, 0x4000
    mov dx, 6
    div cx
    jmp .wall_height_done
    .wall_height_sat:
    mov ax, 0xffff

    .wall_height_done:
    mov bp, ax ;bp=wall_height
    mov cx, ax
    shr cx, 1
    xor dx, dx
    cmp cx, 0x64
    jae .top_done
    mov dx, 0x64
    sub dx, cx
    .top_done:
    mov ax, bp
    sub ax, cx
    cmp ax, 0x64
    jae .bottom_clipped
    add ax, 0x64
    jmp .bottom_done
    .bottom_clipped:
    mov ax, 0xc8
    .bottom_done:
    sub ax, 0x64
    push ax
    mov ax, dx
    mov cx, ax
    shl ax, 6
    xchg cl, ch
    add ax, cx
    add ax, si
    mov di, ax ; di=framebuffer offset
    mov si, dx
    sub si, 0x64
    mov ax, [ray_hit_q6]
    shr ax, 5
    and ax, 0x3f
    cmp byte [ray_side], 0
    jne .texture_x_done
    xor ax, 0x3f
    .texture_x_done:
    mov bx, ax ; bx=texture_x
    .y_loop:
    mov cx, si
    mov ax, cx
    test ax, ax
    jns .delta_positive
    neg ax
    .delta_positive:
    shl ax, 6
    xor dx, dx
    div bp
    test cx, cx
    jns .texture_lower_half
    cmp ax, 0x20
    jae .texture_upper_zero
    mov dx, 0x20
    sub dx, ax
    mov ax, dx
    jmp .texture_y_clamp

    .texture_upper_zero:
    xor ax, ax
    jmp .texture_y_done

    .texture_lower_half:
    add ax, 0x20

    .texture_y_clamp:
    cmp ax, 0x3f
    jbe .texture_y_done
    mov ax, 0x3f

    .texture_y_done:
    mov dx, ax
    mov ax, bx
    mov cx, [ray_distance_q6]
    call wall_pixel_color
    mov [es:di], al
    mov dx, [ray_distance_q6]
    mov [gs:di], dl
    mov [fs:di], dh
    add di, 0x140
    inc si
    pop ax
    cmp si, ax
    jge .wall_done
    push ax
    jmp .y_loop
    .wall_done:
    pop bp ;camera_q14
    pop bx ;cos_view
    pop di ;sin_view
    pop si ;x
    .no_hit:
    pop ax
    add bp, 0x66
    add al, 2
    cmp al, 5
    jb .camera_done
    inc bp
    sub al, 5
    .camera_done:
    push ax
    inc si
    jmp .x_loop
    .done:
    pop ax
    pop bp
    pop bx
    pop di
    pop si
    ret

project_model_vertex:
    push si
    push di
    push bx
    push bp
    xor dh, dh
    mov bp, dx
    mov bx, ax
    xor ax, ax
    mov al, [model_x8 + bx]
    sub ax, 0x52
    shl ax, 6
    add ax, 0x3100
    sub ax, [player_x]
    mov si, ax ;si=translated_x
    xor ax, ax
    mov al, [model_y8 + bx]
    sub ax, 0x10
    shl ax, 6
    mov di, ax ;di=world_y
    xor ax, ax
    mov al, [model_z8 + bx]
    sub ax, 0x40
    shl ax, 6
    add ax, 0x2d00
    sub ax, [player_y]
    mov bx, ax ;bx=translated_z
    mov ax, si
    mov dx, [model_cos]
    call multiply_shift14
    push ax
    mov ax, bx
    mov dx, [model_sin]
    call multiply_shift14
    cmp ax, 0x8000
    jne .rot_x_neg
    mov ax, 0x7fff
    jmp .rot_x_neg_done
    .rot_x_neg:
    neg ax
    .rot_x_neg_done:
    pop dx

    add ax, dx
    jno .rot_x_done
    test dx, dx
    js .rot_x_sat_neg
    mov ax, 0x7fff
    jmp .rot_x_done
    .rot_x_sat_neg:
    mov ax, 0x8000
    .rot_x_done:
    push ax  ;rotated_x


    mov ax, si
    mov dx, [model_sin]
    call multiply_shift14
    push ax
    mov ax, bx
    mov dx, [model_cos]
    call multiply_shift14
    pop dx
    add ax, dx
    jno .rot_z_done
    test dx, dx
    js .rot_z_sat_neg
    mov ax, 0x7fff
    jmp .rot_z_done
    .rot_z_sat_neg:
    mov ax, 0x8000
    
    .rot_z_done:
    mov bx, ax
    pop si
    cmp bx, 0x40
    jle .not_visible
    cmp bx, 0x7fff
    jg .not_visible
    mov ax, si
    mov cx, ax
    test ax, ax
    jns .project_x_abs_done
    neg ax
    .project_x_abs_done:
    mov dx, 0xa0
    mul dx
    cmp dx, bx
    jae .project_x_overflow
    div bx
    test cx, cx
    js .project_x_negative
    cmp ax, 0x7fff
    jbe .project_x_div_done
    mov ax, 0x7fff
    jmp .project_x_div_done
    .project_x_negative:
    cmp ax, 0x8000
    jae .project_x_sat_neg
    neg ax
    jmp .project_x_div_done
    .project_x_overflow:
    test cx, cx
    js .project_x_sat_neg
    mov ax, 0x7fff
    jmp .project_x_div_done
    .project_x_sat_neg:
    mov ax, 0x8000
    .project_x_div_done:
    add ax, 0xa0
    cmp ax, 0xf800
    jge .project_x_min_ok
    mov ax, 0xf800
    .project_x_min_ok:
    cmp ax, 0x7ff
    jle .project_x_max_ok
    mov ax, 0x7ff
    .project_x_max_ok:
    push ax ;projected_x
    mov ax, di
    mov cx, ax
    test ax, ax
    jns .project_y_abs_done
    neg ax
    
    .project_y_abs_done:
    mov dx, 0xa0
    mul dx
    cmp dx, bx
    jae .project_y_overflow
    div bx
    test cx, cx
    js .project_y_negative
    cmp ax, 0x7fff
    jbe .project_y_div_done
    mov ax, 0x7fff
    jmp .project_y_div_done
    .project_y_negative:
    cmp ax, 0x8000
    jae .project_y_sat_neg
    neg ax
    jmp .project_y_div_done
    .project_y_overflow:
    test cx, cx
    js .project_y_sat_neg
    mov ax, 0x7fff
    jmp .project_y_div_done
    .project_y_sat_neg:
    mov ax, 0x8000
    .project_y_div_done:
    add ax, 0x64
    cmp ax, 0xf800
    jge .project_y_min_ok
    mov ax, 0xf800
    .project_y_min_ok:
    cmp ax, 0x7ff
    jle .project_y_max_ok
    mov ax, 0x7ff

    .project_y_max_ok:
    mov di, ax ;di=projected_y
    pop si ;si=projected_x
    test bp, bp
    jnz .slot_not_0
    mov [projected_x0], si
    mov [projected_y0], di
    mov [projected_z0], bx
    jmp .done
    .slot_not_0:
    cmp bp, 1
    jne .slot_2
    mov [projected_x1], si
    mov [projected_y1], di
    mov [projected_z1], bx
    jmp .done
    .slot_2:
    mov [projected_x2], si
    mov [projected_y2], di
    mov [projected_z2], bx
    jmp .done
    .not_visible:
    test bp, bp
    jnz .not_visible_not_0
    mov word [projected_z0], 0
    jmp .done
    .not_visible_not_0:
    cmp bp, 1
    jne .not_visible_2
    mov word [projected_z1], 0
    jmp .done
    .not_visible_2:
    mov word [projected_z2], 0
    .done:
    pop bp
    pop bx
    pop di
    pop si
    ret

cast_ray:
    push si
    push di
    push bx
    push bp

    mov si, ax ;si=direction_x
    mov di, dx ;di=direction_y
    mov byte [ray_hit], 0x00
    mov word [ray_distance_q6], 0xffff

    mov ax, si
    call unsigned_abs
    test ax, ax
    jnz .direction_ok
    mov ax, di
    call unsigned_abs
    test ax, ax
    jz .done
    .direction_ok:
    mov ax, [player_x]
    shr ax, 0xb
    mov [ray_map_x], ax
    mov ax, [player_y]
    shr ax, 0xb
    mov [ray_map_y], ax
    mov ax, si
    call unsigned_abs
    cmp ax, 0x200
    jb .x_small
    mov cx, ax
    mov ax, 0x200
    xor dx, dx
    call div_u32_u16_sat
    mov [ray_delta_x], ax
    mov ax, [player_x]
    and ax, 0x7ff
    test si, si
    jns .x_positive
    mov word [ray_step_x], 0xffff
    mov dx, [ray_delta_x]
    mov cx, 0x800
    call multiply_divide_u16
    mov [ray_side_x], ax
    jmp .x_done
    .x_positive:
    mov word [ray_step_x], 1
    neg ax
    add ax, 0x800
    mov dx, [ray_delta_x]
    mov cx, 0x800
    call multiply_divide_u16
    mov [ray_side_x], ax
    jmp .x_done
    .x_small:
    mov word [ray_delta_x], 0xffff
    mov word [ray_side_x], 0xffff
    mov word [ray_step_x], 1
    .x_done:

    mov ax, di
    call unsigned_abs
    cmp ax, 0x200
    jb .y_small
    mov cx, ax
    mov ax, 0x200
    xor dx, dx
    call div_u32_u16_sat
    mov [ray_delta_y], ax
    mov ax, [player_y]
    and ax, 0x7ff
    test di, di
    jns .y_positive
    mov word [ray_step_y], 0xffff
    mov dx, [ray_delta_y]
    mov cx, 0x800
    call multiply_divide_u16
    mov [ray_side_y], ax
    jmp .y_done
    .y_positive:
    mov word [ray_step_y], 1
    neg ax
    add ax, 0x800
    mov dx, [ray_delta_y]
    mov cx, 0x800
    call multiply_divide_u16
    mov [ray_side_y], ax
    jmp .y_done
    .y_small:
    mov word [ray_delta_y], 0xffff
    mov word [ray_side_y], 0xffff
    mov word [ray_step_y], 1
    .y_done:
    mov bp, 0x20
    .step_loop:
    mov ax, [ray_side_x]
    cmp ax, 0xffff
    jne .have_side
    cmp word [ray_side_y], 0xffff
    je .after_steps
    .have_side:
    mov ax, [ray_side_x]
    cmp ax, [ray_side_y]
    ja .step_y
    mov [ray_distance_q6], ax
    mov dx, [ray_delta_x]
    call saturated_add_u16
    mov [ray_side_x], ax
    mov ax, [ray_step_x]
    add [ray_map_x], ax
    mov byte [ray_side], 0
    jmp .check_cell
    .step_y:
    mov ax, [ray_side_y]
    mov [ray_distance_q6], ax
    mov dx, [ray_delta_y]
    call saturated_add_u16
    mov [ray_side_y], ax
    mov ax, [ray_step_y]
    add [ray_map_y], ax
    mov byte [ray_side], 1
    
    .check_cell:
    mov ax, [ray_map_x]
    mov dx, [ray_map_y]
    call map_cell
    test al, al
    jnz .hit
    dec bp
    jnz .step_loop
    .after_steps:
    jmp .done

    .hit:
    mov byte [ray_hit], 1
    cmp byte [ray_side], 0
    jne .exact_y
    mov ax, [ray_map_x]
    cmp word [ray_step_x], 0
    jg .x_edge_ready
    inc ax
    .x_edge_ready:
    shl ax, 0xb
    sub ax, [player_x]
    mov bx, ax ;bx=edge_delta
    mov ax, si
    call unsigned_abs
    mov bp, ax
    mov ax, bx
    call unsigned_abs
    mov dx, 0x4000
    mov cx, bp
    call multiply_divide_u16
    mov [ray_distance_q6], ax
    mov ax, bx
    mov dx, di
    mov cx, si
    call multiply_divide_s16
    call add_player_y
    mov [ray_hit_q6], ax
    jmp .done

    .exact_y:
    mov ax, [ray_map_y]
    cmp word [ray_step_y], 0
    jg .y_edge_ready
    inc ax
    .y_edge_ready:
    shl ax, 0xb
    sub ax, [player_y]
    mov bx, ax
    mov ax, di
    call unsigned_abs
    mov bp, ax
    mov ax, bx
    call unsigned_abs
    mov dx, 0x4000
    mov cx, bp
    call multiply_divide_u16
    mov [ray_distance_q6], ax
    mov ax, bx
    mov dx, si
    mov cx, di
    call multiply_divide_s16
    call add_player_x
    mov [ray_hit_q6], ax
    
    .done:
    pop bp
    pop bx
    pop di
    pop si
    ret

draw_span:
    push si
    push di
    push bx
    push bp
    mov ax, [span_y]
    test ax, ax
    js .done
    cmp ax, 0xc8
    jae .done
    mov ax, [span_left_q3]
    call q3_to_integer
    mov si, ax
    mov ax, [span_right_q3]
    call q3_to_integer
    mov di, ax
    cmp si, di
    jle .ordered
    xchg si, di
    mov ax, [span_left_z]
    xchg ax, [span_right_z]
    mov [span_left_z], ax

    .ordered:
    mov bp, [span_left_z]
    mov dx, di
    sub dx, si
    test dx, dx
    jle .done
    test di, di
    jle .done
    cmp si, 0x140
    jge .done
    mov ax, [span_right_z]
    sub ax, [span_left_z]
    call divide_s16
    mov bx, ax
    test si, si
    jns .left_clipped
    mov dx, si
    neg dx ;dx=skipped_pixels
    mov ax, bx
    mov cx, 1
    call multiply_divide_s16
    mov dx, ax
    mov ax, bp
    call saturated_add_s16
    mov bp, ax ;bp=depth
    xor si, si

    .left_clipped:
    cmp di, 0x140
    jle .right_clipped
    mov di, 0x140

    .right_clipped:
    cmp di, si
    jle .done
    sub di, si
    mov ax, [span_y]
    mov dx, si
    call screen_offset
    mov si, di
    mov di, ax

    .pixel_loop:
    mov ax, bp
    mov dx, 0x20
    call saturated_add_s16
    cmp ax, 1
    jge .depth_min_ok
    mov ax, 1
    .depth_min_ok:
    cmp ax, 0x7fff
    jle .depth_max_ok
    mov ax, 0x7fff

    .depth_max_ok:
    push ax
    mov ax, di
    call depth_get
    pop dx ;dx=stored_depth
    cmp dx, ax
    jae .skip_pixel
    mov [gs:di], dl
    mov [fs:di], dh
    mov al, [span_color]
    mov [es:di], al
    
    .skip_pixel:
    mov ax, bp
    mov dx, bx
    call saturated_add_s16
    mov bp, ax
    inc di
    dec si
    jnz .pixel_loop

    .done:
    pop bp
    pop bx
    pop di
    pop si
    ret

GLOBAL rasterize_triangle
rasterize_triangle:
    push si
    push di
    push bx
    push bp
    mov [span_color], al
    mov ax, [projected_y1]
    cmp ax, [projected_y0]
    jge .sort_12
    call swap_vertices_01
    .sort_12:
    mov ax, [projected_y2]
    cmp ax, [projected_y1]
    jge .sort_01_again
    call swap_vertices_12
    .sort_01_again:
    mov ax, [projected_y1]
    cmp ax, [projected_y0]
    jge .sorted
    call swap_vertices_01
    .sorted:
    mov bp, [projected_y2]
    sub bp, [projected_y0]
    test bp, bp
    jle .done
    mov ax, [projected_x0]
    and ax, [projected_x1]
    and ax, [projected_x2]
    js .done
    cmp word [projected_x0], 0x140
    jl .x_visible
    cmp word [projected_x1], 0x140
    jl .x_visible
    cmp word [projected_x2], 0x140
    jge .done
    .x_visible:
    cmp word [projected_y2], 0
    jl .done
    cmp word [projected_y0], 0xc8
    jge .done
    mov ax, [projected_x2]
    sub ax, [projected_x0]
    mov dx, bp
    call divide_q3
    mov si, ax
    mov ax, [projected_z2]
    sub ax, [projected_z0]
    mov dx, bp
    call divide_s16
    mov di, ax
    mov bp, [projected_y2]
    sub bp, [projected_y1]
    test bp, bp
    jz .slope_23_zero
    mov ax, [projected_x2]
    sub ax, [projected_x1]
    mov dx, bp
    call divide_q3
    mov bx, ax ;bx=slope_x_23
    mov ax, [projected_z2]
    sub ax, [projected_z1]
    mov dx, bp
    call divide_s16 ;ax=slope_z_23
    jmp .slope_23_done
    .slope_23_zero:
    xor bx, bx
    xor ax, ax
    .slope_23_done:
    push bx ;slope_x_23
    push ax ;slope_z_23
    mov bp, [projected_y1]
    sub bp, [projected_y0]
    test bp, bp
    jz .slope_12_zero
    mov ax, [projected_x1]
    sub ax, [projected_x0]
    mov dx, bp
    call divide_q3
    mov bx, ax
    mov ax, [projected_z1]
    sub ax, [projected_z0]
    mov dx, bp
    call divide_s16
    mov bp, ax
    jmp .slope_12_done
    .slope_12_zero:
    xor bx, bx
    xor bp, bp
    .slope_12_done:
    mov ax, [projected_y2]
    call .clamp_y
    push ax ;end_y
    mov ax, [projected_y1]
    call .clamp_y
    push ax ;middle_y
    mov ax, [projected_y0]
    call .clamp_y
    mov [span_y], ax
    mov cx, [span_y]
    sub cx, [projected_y0]
    push cx
    mov ax, [projected_x0]
    mov dx, si
    pop cx
    push cx
    call edge_start_q3
    mov [span_left_q3], ax
    mov ax, [projected_z0]
    mov dx, di
    pop cx
    push cx
    call edge_start_z
    mov [span_left_z], ax
    mov ax, [projected_x0]
    mov dx, bx
    pop cx
    push cx
    call edge_start_q3
    mov [span_right_q3], ax
    mov ax, [projected_z0]
    mov dx, bp
    pop cx
    call edge_start_z
    mov [span_right_z], ax
   
    .upper_loop:
    pop ax
    cmp [span_y], ax
    jge .upper_done
    push ax
    push word [span_left_z]
    push word [span_right_z]
    call draw_span
    pop word [span_right_z]
    pop word [span_left_z]
    mov ax, [span_left_q3]
    mov dx, si
    call saturated_add_s16
    mov [span_left_q3], ax
    mov ax, [span_left_z]
    mov dx, di
    call saturated_add_s16
    mov [span_left_z], ax
    mov ax, [span_right_q3]
    mov dx, bx
    call saturated_add_s16
    mov [span_right_q3], ax
    mov ax, [span_right_z]
    mov dx, bp
    call saturated_add_s16
    mov [span_right_z], ax
    inc word [span_y]
    jmp .upper_loop

    .upper_done:
    mov [span_y], ax ;y=middle_y
    pop cx ;end_y
    pop bp ;slope_z_23
    pop bx ;slope_x_23
    push cx ;end_y
    ; si=slope_x_13
    ; di=slope_z_13
    ; bx=slope_x_23
    ; bp=slope_z_23
    mov cx, [span_y]
    sub cx, [projected_y0]
    push cx
    mov ax, [projected_x0]
    mov dx, si
    pop cx
    push cx
    call edge_start_q3
    mov [span_left_q3], ax
    mov ax, [projected_z0]
    mov dx, di
    pop cx
    call edge_start_z
    mov [span_left_z], ax
    mov cx, [span_y]
    sub cx, [projected_y1]
    push cx
    mov ax, [projected_x1]
    mov dx, bx
    pop cx
    push cx
    call edge_start_q3
    mov [span_right_q3], ax
    mov ax, [projected_z1]
    mov dx, bp
    pop cx
    call edge_start_z
    mov [span_right_z], ax

    .lower_loop:
    pop ax
    cmp [span_y], ax
    jge .lower_done
    push ax
    push word [span_left_z]
    push word [span_right_z]
    call draw_span
    pop word [span_right_z]
    pop word [span_left_z]
    mov ax, [span_left_q3]
    mov dx, si
    call saturated_add_s16
    mov [span_left_q3], ax
    mov ax, [span_left_z]
    mov dx, di
    call saturated_add_s16
    mov [span_left_z], ax
    mov ax, [span_right_q3]
    mov dx, bx
    call saturated_add_s16
    mov [span_right_q3], ax
    mov ax, [span_right_z]
    mov dx, bp
    call saturated_add_s16
    mov [span_right_z], ax
    inc word [span_y]
    jmp .lower_loop
    
    .lower_done:
    .done:
    pop bp
    pop bx
    pop di
    pop si
    ret

    .clamp_y:
    test ax, ax
    jns .clamp_y_positive
    xor ax, ax
    ret
    .clamp_y_positive:
    cmp ax, 0xc8
    jle .clamp_y_done
    mov ax, 0xc8
    .clamp_y_done:
    ret

collision_blocks_x:
    ; if no collision (kinda the solution to this challenge):
    ; xor ax, ax 
    ; ret
    push bx
    push di
    push si
    not cx
    shr cx, 0x6
    and cx, 0x200
    add ax, cx
    add ax, 0xff00
    mov si, ax
    shr si, 0xb
    mov ax, dx
    add ax, 0xff00
    mov cx, ax
    shr cx, 0xb
    add dx, 0x100
    mov di, dx
    shr di, 0xb
    mov ax, si
    mov dx, cx
    call map_cell
    mov bx, ax
    mov ax, si
    mov dx, di
    call map_cell
    or al, bl
    pop si
    pop di
    pop bx
    ret
    
collision_blocks_y:
    ; if no collision (kinda the solution to this challenge):
    ; xor ax, ax 
    ; ret
    push bx
    push di
    push si
    not cx
    shr cx, 0x6
    and cx, 0x200
    add cx, dx
    add cx, 0xff00
    mov si, cx
    shr si, 0xb
    mov cx, ax
    add cx, 0xff00
    mov cx, cx
    shr cx, 0xb
    add ax, 0x100
    mov di, ax
    shr di, 0xb
    mov ax, cx
    mov dx, si
    call map_cell
    mov bx, ax
    mov ax, di
    mov dx, si
    call map_cell
    or al, bl
    pop si
    pop di
    pop bx
    ret
    
move_player:
    push bp
    push bx
    push di
    push si
    
    mov ax, word [view_angle]
    call sine_q14
    mov si, ax ;si=sine
    
    mov ax, word [view_angle]
    call cosine_q14
    mov di, ax ;di=cosine
    
    xor bx, bx ; move_x
    xor bp, bp ; move_y
    
    mov cl, byte [held_keys]
    
    test cl, 1
    jz .n0
    add bx, si
    add bp, di
    .n0:
    test cl, 2
    jz .n1
    sub bx, si
    sub bp, di
    .n1:
    test cl, 4
    jz .n2
    sub bx, di
    add bp, si
    .n2:
    test cl, 8
    jz .n3
    add bx, di
    sub bp, si
    .n3:
    test bx, bx
    jnz .cont
    test bp, bp
    jz .done
    .cont:
    
    test cl, 3
    jz .no_adjust
    and cl, 0xc
    jz .no_adjust
    
    mov ax, bx
    mov dx, 0x2d41 ;INV_SQRT2_Q14
    mov cx, 0x4000
    call multiply_divide_s16
    mov bx, ax 
    
    mov ax, bp
    mov dx, 0x2d41 ;INV_SQRT2_Q14
    mov cx, 0x4000
    call multiply_divide_s16
    mov bp, ax 
    
    .no_adjust:
    
    mov ax, bx
    mov dx, 0x140
    mov cx, 0x4000
    call multiply_divide_s16
    add ax, word [player_x]
    mov si, ax ;new_x
    
    mov ax, bp
    mov dx, 0x140
    mov cx, 0x4000
    call multiply_divide_s16
    add ax, word [player_y]
    mov di, ax ;new_y
    
    mov ax, si
    mov dx, word [player_y]
    mov cx, bx
    call collision_blocks_x
    test ax, ax
    jnz .no_move_x
    mov word [player_x], si
    .no_move_x:
    
    mov ax, word [player_x]
    mov dx, di
    mov cx, bp
    call collision_blocks_y
    test ax, ax
    jnz .no_move_y
    mov word [player_y], di
    .no_move_y:
    
    .done:
    pop si
    pop di
    pop bx
    pop bp
    ret

kernel_main:
    mov word [player_x], 0x2000
    mov word [player_y], 0x1000
    mov word [view_angle], 0x0
    mov byte [held_keys], 0x0
    mov byte [mouse_phase], 0x0
    mov byte [keyboard_extended], 0x0
    mov word [pending_mouse_dx], 0x0
    call initialize_palette
    call initialize_mouse
    lea si, [si]
    .loop:
    call read_pit_counter
    mov word [frame_start_pit], ax
    call poll_input
    call apply_mouse_rotation
    call move_player
    call depth_clear
    call render_sky
    call render_floor
    call render_walls
    call render_model
    call wait_vertical_retrace
    call present_frame
    call limit_frame_rate
    jmp .loop


align 2, db 0

sine_quarter_q14:
    dw 0, 402, 804, 1205, 1606, 2006, 2404, 2801, 3196, 3590, 3981, 4370
    dw 4756, 5139, 5520, 5897, 6270, 6639, 7005, 7366, 7723, 8076, 8423, 8765
    dw 9102, 9434, 9760, 10080, 10394, 10702, 11003, 11297, 11585, 11866, 12140, 12406
    dw 12665, 12916, 13160, 13395, 13623, 13842, 14053, 14256, 14449, 14635, 14811, 14978
    dw 15137, 15286, 15426, 15557, 15679, 15791, 15893, 15986, 16069, 16143, 16207, 16261
    dw 16305, 16340, 16364, 16379, 16384
model_x8:
    db 56, 56, 62, 62, 51, 51, 51, 52, 51, 51, 52, 52, 52, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54
    db 54, 54, 55, 54, 55, 55, 55, 55, 54, 55, 55, 55, 48, 48, 50, 49, 45, 46, 41, 41, 42, 42, 43, 44
    db 44, 43, 43, 42, 42, 43, 43, 43, 45, 45, 45, 36, 37, 38, 36, 37, 37, 38, 38, 39, 39, 39, 39, 38
    db 38, 38, 37, 37, 37, 40, 40, 40, 40, 41, 41, 57, 58, 58, 57, 58, 56, 56, 58, 57, 57, 58, 57, 59
    db 59, 58, 59, 59, 59, 60, 60, 60, 61, 60, 61, 61, 57, 58, 59, 61, 62, 62, 62, 62, 63, 63, 64, 64
    db 64, 64, 63, 63, 63, 51, 51, 51, 52, 52, 51, 51, 52, 51, 52, 53, 53, 53, 53, 53, 53, 52, 52, 52
    db 54, 54, 54, 54, 54, 53, 54, 55, 55, 55, 55, 54, 55, 55, 55, 54, 55, 43, 44, 44, 43, 44, 42, 42
    db 44, 43, 43, 44, 43, 45, 45, 44, 45, 45, 45, 46, 46, 46, 47, 46, 47, 47, 43, 44, 45, 47, 48, 48
    db 48, 48, 49, 49, 50, 50, 50, 50, 49, 49, 49, 37, 38, 38, 37, 38, 38, 42, 42, 41, 41, 41, 41, 0
    db 144, 144, 0, 104, 103, 103, 105, 105, 104, 104, 105, 107, 107, 106, 106, 106, 106, 107, 106, 106, 106, 106, 105, 105
    db 104, 105, 104, 104, 104, 105, 107, 107, 107, 107, 107, 108, 108, 108, 109, 109, 109, 108, 109, 108, 109, 110, 110, 109
    db 108, 109, 107, 108, 107, 96, 96, 98, 98, 99, 100, 100, 100, 99, 98, 99, 98, 99, 99, 99, 101, 101, 102, 102
    db 103, 103, 101, 102, 102, 89, 89, 91, 91, 93, 94, 93, 93, 92, 92, 91, 91, 93, 93, 93, 94, 95, 95, 95
    db 96, 96, 96, 80, 80, 82, 85, 85, 85, 89, 89, 87, 87, 86, 87, 85, 87, 65, 67, 67, 65, 67, 67, 68
    db 69, 69, 68, 68, 70, 70, 72, 70, 71, 71, 73, 73, 75, 75, 74, 74, 73, 74, 76, 75, 76, 76, 78, 78
    db 78, 76, 78, 77, 77, 71, 71, 73, 71, 115, 115, 113, 115, 113, 115, 116, 116, 116, 117, 117, 117, 117, 117, 116
    db 116, 118, 118, 118, 118, 119, 119, 121, 121, 119, 119, 121, 121, 119, 120, 119, 120, 119, 120, 120, 105, 105, 105, 106
    db 107, 106, 105, 105, 107, 108, 108, 108, 108, 107, 107, 107, 108, 107, 107, 107, 106, 105, 105, 106, 106, 109, 109, 108
    db 109, 108, 109, 109, 110, 110, 111, 111, 109, 110, 110, 111, 111, 111, 109, 111, 110, 110, 108, 108, 98, 99, 99, 98
    db 99, 99, 100, 100, 100, 101, 101, 102, 102, 103, 104, 104, 103, 99, 87, 87, 88, 88, 88, 88, 88, 88, 89, 89
    db 89, 89, 90, 90, 90, 90, 90, 90, 91, 91, 91, 91, 92, 91, 91, 92, 93, 92, 93, 92, 92, 92, 92, 93
    db 93, 94, 92, 94, 93, 94, 93, 95, 95, 95, 93, 94, 92, 94, 94, 93, 94, 94, 92, 94, 94, 94, 94, 92
    db 79, 79, 77, 79, 77, 79, 80, 80, 80, 81, 81, 82, 82, 82, 82, 81, 81, 80, 83, 83, 83, 83, 83, 85
    db 85, 84, 84, 85, 85, 84, 83, 85, 83, 85, 84, 70, 70, 71, 72, 71, 72, 72, 73, 73, 72, 72, 73, 75
    db 75, 76, 76, 76, 76, 75, 75, 74, 74, 74, 74, 74, 74, 73, 73, 73, 72, 72, 74, 72, 74, 72, 72, 74
    db 74, 73, 73, 74, 72, 74, 72, 74, 74, 72, 74, 72, 74, 72, 111, 111, 111, 111, 112, 112, 112, 112, 113, 113
    db 112, 115, 115, 116, 116, 115, 114, 115, 113, 114, 114, 115, 113, 114, 114, 113, 114, 113, 114, 113, 114, 114, 113, 113
    db 115, 113, 115, 113, 114, 114, 113, 113, 114, 113, 114, 113, 114, 114, 144, 144, 0, 0
model_y8:
    db 29, 30, 30, 29, 26, 28, 27, 27, 20, 21, 21, 28, 20, 28, 27, 23, 24, 21, 20, 28, 27, 25, 24, 23
    db 21, 20, 27, 26, 26, 25, 24, 23, 22, 22, 21, 20, 26, 28, 23, 23, 23, 23, 23, 24, 24, 26, 26, 27
    db 28, 28, 27, 28, 27, 24, 23, 21, 28, 27, 24, 26, 26, 26, 25, 25, 24, 25, 24, 28, 27, 23, 24, 28
    db 27, 23, 28, 27, 23, 28, 27, 24, 23, 28, 27, 14, 14, 13, 13, 12, 12, 11, 11, 11, 10, 10, 9, 16
    db 15, 15, 14, 9, 8, 15, 9, 8, 16, 16, 9, 8, 15, 9, 10, 15, 9, 8, 15, 16, 9, 8, 15, 14
    db 10, 8, 14, 15, 16, 14, 15, 16, 16, 15, 10, 11, 10, 9, 9, 16, 15, 8, 9, 12, 11, 12, 11, 8
    db 9, 8, 16, 15, 11, 13, 12, 9, 10, 8, 15, 14, 14, 13, 16, 13, 12, 14, 14, 13, 13, 12, 12, 11
    db 11, 11, 10, 10, 9, 16, 15, 15, 14, 9, 8, 15, 9, 8, 16, 16, 9, 8, 15, 9, 10, 15, 9, 8
    db 15, 16, 9, 8, 15, 14, 10, 8, 14, 15, 16, 16, 15, 12, 8, 11, 9, 16, 15, 12, 11, 9, 8, 40
    db 40, 0, 0, 34, 35, 33, 36, 34, 36, 29, 29, 28, 27, 28, 35, 36, 27, 30, 30, 32, 29, 34, 32, 31
    db 31, 30, 30, 28, 27, 27, 35, 36, 34, 31, 32, 31, 27, 29, 27, 29, 28, 36, 35, 34, 34, 34, 33, 33
    db 33, 32, 33, 32, 29, 28, 29, 29, 33, 33, 35, 36, 34, 36, 35, 34, 34, 29, 27, 25, 35, 36, 36, 34
    db 36, 34, 34, 29, 27, 28, 29, 29, 33, 33, 35, 36, 34, 36, 35, 35, 34, 29, 27, 25, 36, 36, 35, 34
    db 36, 34, 27, 31, 32, 30, 32, 30, 26, 32, 30, 30, 32, 22, 22, 36, 36, 36, 36, 29, 27, 28, 27, 29
    db 28, 27, 27, 28, 27, 28, 28, 29, 28, 27, 29, 28, 28, 27, 28, 27, 27, 29, 27, 29, 30, 36, 36, 30
    db 29, 29, 28, 28, 27, 30, 36, 36, 29, 21, 16, 21, 14, 7, 9, 15, 14, 16, 15, 16, 14, 9, 7, 9
    db 7, 14, 16, 9, 7, 16, 8, 13, 12, 12, 11, 11, 10, 15, 15, 13, 14, 10, 9, 8, 14, 15, 13, 16
    db 14, 14, 16, 9, 9, 8, 7, 15, 16, 16, 15, 7, 10, 12, 8, 10, 11, 10, 8, 8, 7, 7, 16, 14
    db 11, 12, 8, 9, 9, 7, 9, 8, 14, 16, 15, 15, 14, 13, 13, 12, 12, 11, 13, 11, 16, 16, 11, 7
    db 9, 10, 10, 9, 8, 7, 9, 7, 9, 9, 8, 7, 7, 7, 13, 15, 13, 14, 16, 3, 5, 4, 16, 14
    db 4, 2, 16, 14, 8, 10, 2, 4, 16, 14, 10, 8, 16, 4, 2, 14, 16, 10, 9, 7, 8, 4, 5, 3
    db 15, 15, 13, 14, 13, 13, 12, 13, 12, 11, 11, 11, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 3, 2
    db 21, 16, 21, 14, 7, 9, 16, 15, 14, 16, 15, 16, 14, 9, 7, 9, 7, 7, 16, 7, 14, 9, 8, 13
    db 12, 12, 11, 11, 10, 15, 13, 14, 10, 9, 8, 12, 10, 12, 11, 10, 12, 13, 12, 11, 10, 9, 10, 20
    db 19, 20, 19, 3, 1, 3, 1, 20, 19, 2, 3, 1, 18, 20, 2, 19, 19, 18, 17, 17, 15, 15, 14, 14
    db 13, 13, 9, 9, 8, 8, 7, 7, 6, 5, 5, 4, 4, 3, 39, 40, 23, 21, 40, 39, 23, 21, 40, 22
    db 38, 32, 31, 32, 30, 30, 31, 33, 32, 32, 30, 29, 30, 40, 39, 38, 38, 37, 37, 35, 35, 34, 34, 33
    db 34, 29, 28, 28, 28, 27, 27, 26, 25, 25, 24, 24, 23, 22, 40, 0, 40, 0
model_z8:
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
    db 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 80
    db 80, 80, 80, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74
    db 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 0, 0, 0, 0
tri_p0:
    dw 1024, 3074, 5124, 5124, 5125, 5125, 7175, 5126, 9224, 10249, 10250, 8200
    dw 7179, 7175, 11275, 10250, 12300, 10250, 12300, 12298, 7179, 7175, 11275, 7175
    dw 11269, 5127, 10248, 8204, 10250, 12300, 10250, 12298, 14349, 14349, 14350, 13323
    dw 11278, 16399, 15376, 17420, 12306, 17425, 18449, 14349, 15376, 14355, 13326, 14355
    dw 16399, 15376, 17425, 17426, 14349, 13326, 15376, 16399, 18449, 17426, 14347, 11275
    dw 11271, 10252, 12300, 17418, 12305, 19476, 20500, 21525, 23575, 24600, 25625, 19476
    dw 20500, 26643, 20500, 26650, 20507, 26650, 27675, 26650, 27675, 28698, 27675, 28700
    dw 27675, 28700, 27669, 29724, 29725, 21525, 29725, 29725, 21525, 29725, 30749, 21525
    dw 30750, 30750, 21525, 30750, 30750, 21525, 22558, 21526, 22551, 31766, 23575, 31775
    dw 31775, 23575, 31775, 31775, 23584, 31775, 31775, 32800, 33823, 33825, 32800, 33825
    dw 33825, 32800, 33825, 32800, 34849, 32800, 34850, 32792, 34850, 35874, 24600, 35875
    dw 24600, 24601, 19470, 20499, 22549, 16400, 16399, 15383, 23574, 25624, 25625, 17425
    dw 25625, 17425, 17426, 37924, 39974, 36901, 37928, 41001, 44074, 46124, 48175, 48176
    dw 48175, 48177, 51247, 50225, 51250, 50225, 51250, 50227, 51250, 52276, 53300, 51250
    dw 53300, 51250, 53300, 53300, 47154, 53293, 46126, 47148, 55349, 45098, 56374, 49199
    dw 49199, 49200, 48176, 49200, 48176, 49199, 49200, 48175, 49200, 48176, 57400, 58424
    dw 58424, 57391, 48185, 57400, 58425, 57400, 58425, 58424, 49199, 48175, 57400, 48184
    dw 49199, 48176, 54326, 41018, 61499, 61501, 60475, 61500, 60478, 64572, 63550, 64575
    dw 63550, 64575, 63552, 1087, 2113, 64, 64, 4163, 3140, 5189, 4163, 7236
    dw 6213, 5190, 4167, 8260, 7239, 7240, 2121, 9289, 6210, 5190, 8263, 8264
    dw 10315, 62536, 11339, 62525, 11339, 11324, 61501, 2112, 76, 2114, 12364, 2114
    dw 12361, 2114, 9282, 5189, 5189, 3140, 4173, 3140, 4163, 5190, 5189, 13390
    dw 14413, 15439, 15440, 13380, 14413, 16463, 16464, 6213, 13389, 14414, 18513, 17489
    dw 14414, 18513, 43051, 13390, 14414, 13389, 17485, 14417, 43087, 15439, 16426, 15439
    dw 16464, 13390, 14414, 13389, 14413, 16463, 16464, 15439, 16464, 16464, 15439, 15440
    dw 4163, 3140, 5189, 5189, 19539, 21588, 19542, 21589, 22614, 23637, 22616, 24664
    dw 23639, 24665, 26711, 25691, 26714, 27740, 26714, 28764, 29786, 28766, 32863, 34912
    dw 35939, 36963, 31845, 40038, 37992, 44138, 39015, 31845, 37993, 40038, 39015, 37983
    dw 31840, 35940, 40038, 34913, 34914, 33900, 20578, 45139, 19540, 29790, 30829, 47197
    dw 46189, 35950, 36963, 48232, 43115, 50288, 41071, 48232, 43115, 50282, 52338, 51315
    dw 50288, 49265, 41071, 48242, 52339, 51315, 50288, 50289, 49265, 54388, 53365, 56438
    dw 58488, 56438, 58484, 53365, 61563, 60539, 61564, 60539, 55420, 55414, 59510, 54388
    dw 54389, 53364, 53365, 52338, 51314, 61555, 61564, 60540, 54388, 50293, 49264, 49265
    dw 48232, 41061, 44138, 43115, 62589, 62589, 63613, 127, 1151, 1153, 64638, 1153
    dw 63614, 63613, 3202, 2178, 4228, 2178, 4228, 2181, 6276, 5253, 1152, 8327
    dw 8328, 8327, 10377, 7304, 8327, 10377, 9354, 8327, 129, 12427, 13454, 6287
    dw 15503, 10378, 9354, 1152, 1153, 128, 1153, 128, 129, 14477, 13443, 4238
    dw 3203, 3204, 6277, 5253, 6278, 5253, 6278, 5263, 6278, 15503, 6278, 15494
    dw 9354, 17552, 17552, 16529, 8328, 9360, 10377, 16529, 7304, 8339, 18578, 19602
    dw 21652, 17552, 16529, 24727, 23703, 26778, 27803, 28826, 27803, 28828, 27803, 28828
    dw 27803, 29852, 29853, 25751, 23696, 25753, 25753, 16528, 16537, 18579, 19603, 30866
    dw 19603, 26782, 19603, 26778, 19611, 26778, 27802, 29851, 32925, 31903, 32928, 31893
    dw 21654, 25744, 25753, 17561, 16528, 17553, 16528, 16529, 8327, 7304, 20629, 12436
    dw 11403, 11404, 10377, 9354, 33953, 36002, 33956, 36003, 37028, 38051, 37030, 39078
    dw 38053, 39079, 41125, 40105, 41128, 42154, 41128, 43178, 44200, 43180, 47277, 49326
    dw 50353, 51377, 46259, 54452, 52406, 58552, 53429, 46259, 52407, 54452, 53429, 52397
    dw 46254, 50354, 54452, 49327, 49328, 48314, 34992, 59553, 33954, 44204, 45243, 61611
    dw 60603, 50364, 51377, 62646, 57529, 64702, 55485, 62646, 57529, 64696, 1216, 193
    dw 64702, 63679, 55485, 62656, 1217, 193, 64702, 64703, 63679, 3266, 2243, 5316
    dw 7366, 5316, 7362, 2243, 10441, 9417, 10442, 9417, 4298, 4292, 8388, 3266
    dw 3267, 2242, 2243, 1216, 192, 10442, 10442, 9418, 3266, 64707, 63678, 63679
    dw 62646, 55475, 58552, 57529, 12491, 11470, 15565, 16591, 11468, 18641, 13519, 20691
    dw 22741, 16590, 24791, 23770, 27867, 28893, 31966, 31967, 30942, 31966, 31967, 32992
    dw 31967, 32992, 27871, 32988, 28891, 34017, 35042, 34017, 37091, 40166, 39143, 40166
    dw 38117, 37093, 39143, 40167, 40166, 43241, 38124, 38117, 41189, 39143, 46310, 30942
    dw 30943, 43243, 43242, 43242, 47343, 43242, 48368, 43242, 50410, 49394, 35057, 51442
    dw 35042, 51425, 34018, 35041, 34035, 35042, 52467, 52467, 45282, 52468, 45292, 53493
    dw 45292, 54517, 45292, 54517, 45292, 41196, 56566, 55542, 55543, 37091, 37092, 36067
    dw 36068, 56566, 55544, 59641, 61667, 37116, 36068, 62717, 61694, 255, 255, 64765
    dw 63744, 62717, 62718, 56568, 57592, 1281, 57592, 1281, 57592, 2305, 57603, 2306
    dw 3331, 2306, 2306, 3331, 4354, 3331, 5380, 3331, 5381, 6405, 6406, 7430
    dw 3336, 7431, 9479, 9481, 9481, 9481, 8458, 11529, 60683, 10506, 60667, 10490
    dw 59643, 61693, 61692, 61692, 62732, 61692, 12515, 36092, 40166, 55543, 58618, 58617
    dw 44267, 44265, 37091, 36068, 14605, 16655, 18706, 18707, 20754, 20755, 20756, 21781
    dw 20756, 21781, 20756, 21782, 20756, 22806, 22806, 23828, 22808, 23831, 24856, 24856
    dw 17687, 24848, 16657, 17679, 26905, 15629, 27930, 19730, 19740, 29981, 28957, 31005
    dw 28958, 29980, 29981, 28956, 29981, 28957, 31006, 34080, 32032, 31007, 32031, 31006
    dw 32031, 33054, 32031, 32032, 31004, 28962, 31006, 32030, 19730, 18707, 25882, 37155
    dw 39205, 41255, 43306, 43307, 45354, 45355, 45356, 44333, 45356, 46381, 45356, 46382
    dw 45356, 47407, 48431, 45356, 48432, 45356, 49456, 49456, 42284, 49448, 41257, 42279
    dw 51505, 40229, 52530, 53546, 53546, 53556, 43316, 54581, 55605, 53546, 54580, 43306
    dw 54581, 43317, 54581, 58680, 58680, 54583, 56631, 57656, 56631, 57656, 56631, 58680
    dw 54582, 55607, 54581, 56629, 44330, 43316, 50482, 59662, 61755, 61757, 64830, 62779
    dw 2369, 3393, 325, 7486, 4424, 63807, 3396, 323, 5440, 315, 3392, 10569
    dw 9548, 13643, 14669, 11597, 11595, 11595, 15691, 13647, 17744, 16720, 17744, 15698
    dw 18770, 19795, 18770, 16723, 18768, 15693, 13645, 15695, 13651, 15695, 18767, 16724
    dw 16721, 17745, 20816, 20820, 20821, 20820, 21844, 22870, 22871, 24918, 25944, 23895
    dw 25945, 25945, 23895, 25945, 23893, 21844, 16721, 17744, 26966, 26970, 26970, 22870
    dw 26970, 22875, 30044, 29022, 30044, 31072, 33119, 31070, 32095, 31070, 32094, 26971
    dw 27995, 34138, 27995, 34145, 28000, 31073, 33118, 29021, 35164, 29021, 36188, 29026
    dw 38244, 40294, 37220, 40295, 40292, 41319, 37220, 41320, 37225, 41320, 43368, 42345
    dw 44394, 42345, 44395, 45419, 42345, 35180, 42339, 36194, 30044, 29021, 47469, 26991
    dw 46445, 46445, 46448, 22874, 49520, 22870, 49495, 23894, 51569, 53620, 53620, 51571
    dw 52597, 55668, 51570, 57719, 51570, 53624, 53618, 53620, 51570, 53620, 51572, 56697
    dw 58743, 56697, 58745, 58743, 58743, 59769, 58746, 61819, 63869, 63870, 56697, 58743
    dw 63869, 64896, 56697, 56695, 58738, 51570, 51575, 383, 55669, 2433, 1409, 4483
    dw 2433, 1409, 3459, 3460, 8583, 9609, 8584, 9609, 9610, 11656, 10634, 12683
    dw 5505, 1409, 14733, 1423, 16782, 15759, 16784, 7568, 15759, 7559, 9607, 12682
    dw 12684, 17809, 18828, 17809, 18834, 19858, 17795, 6547, 3459, 3462, 60796, 1410
    dw 62846, 4483, 20884, 21910, 24983, 24984, 23959, 24983, 26008, 23962, 26009, 27034
    dw 26009, 27029, 21908, 28059, 29084, 28059, 31133, 33183, 32160, 34210, 30109, 31133
    dw 35233, 34209, 34210, 37284, 39324, 30118, 36253, 35233, 24994, 34199, 23960, 37285
    dw 40356, 40359, 38312, 40359, 41384, 40359, 40359, 41385, 29095, 42409, 29084, 42395
    dw 28060, 29083, 28074, 29084, 43434, 43435, 29084, 44460, 29084, 45484, 29084, 45484
    dw 29084, 45475, 29084, 36252, 33183, 32159, 32160, 46493, 31149, 30109, 30110, 33183
    dw 32175, 50608, 46514, 46509, 30125, 53683, 54709, 56758, 56756, 53684, 54711, 53684
    dw 53685, 47535, 48568, 58798, 57784, 58809, 57784, 59833, 57784, 59834, 57784, 60858
    dw 60859, 57784, 61883, 57784, 61884, 57784, 61884, 62908, 62909, 62909, 57790, 62909
    dw 64957, 64959, 447, 448, 63934, 448, 1472, 63938, 49601, 2481, 50608, 54707
    dw 54709, 46517, 52659, 46509, 52658, 51629, 33183, 32160, 49585, 3504, 50597, 38308
    dw 31133, 30110, 5572, 4551, 6599, 6598, 7624, 9670, 10697, 10698, 8650, 11723
    dw 12748, 13773, 11724, 12748, 14798, 12748, 14798, 13774, 10696, 8651, 11723, 12747
    dw 16847, 15824, 16847, 15824, 15823, 15823, 16848, 16847, 18897, 17873, 18898, 17873
    dw 18898, 17873, 19922, 19921, 20947, 20948, 16848, 20948, 16848, 20948, 20948, 16848
    dw 16847, 14797, 13773, 13773, 16846, 15824, 8647, 24022, 24024, 27098, 27095, 26073
    dw 24025, 29147, 30172, 30173, 28123, 32222, 32223, 31198, 33248, 34273, 33248, 34273
    dw 34272, 32222, 32223, 27098, 32223, 27098, 27097, 30171, 28123, 33248, 28123, 33248
    dw 34272, 36322, 36322, 36323, 35298, 35299, 38372, 37349, 40422, 39398, 40423, 39399
    dw 42472, 44522, 42476, 41449, 42476, 43499, 44522, 46567, 40422, 36328, 35299, 44522
    dw 38372, 39399, 40422, 36322, 31198, 31199, 33249, 34273, 40423, 39399, 45551, 48623
    dw 50673, 53747, 55797, 56823, 49647, 48623, 57848, 48623, 58872, 48634, 60921, 59900
    dw 60923, 61948, 62971, 61950, 65021, 63998, 511, 63998, 512, 1536, 64002, 1537
    dw 3585, 2564, 5635, 5637, 4612, 5637, 5637, 4612, 6661, 51718, 4593, 51698
    dw 50674, 51699, 7666, 52723, 7687, 8711, 52723, 8712, 8712, 52723, 9736, 9737
    dw 52746, 9737, 11785, 10762, 11787, 11787, 10762, 12811, 10742, 12812, 55798, 13836
    dw 55798, 13837, 14861, 55798, 56846, 55798, 55799, 45545, 48620, 51697, 43498, 43499
    dw 44532, 53746, 56821, 15863, 46573, 15887, 46573, 46574, 17936, 19987, 19987, 17938
    dw 18964, 22035, 23062, 25111, 23057, 25112, 25105, 25112, 17937, 19992, 17939, 27161
    dw 23063, 24086, 23062, 23063, 26138, 27161, 26138, 29211, 31261, 33310, 27161, 26138
    dw 33311, 32289, 24086, 24087, 23062, 23062, 23063, 34335, 22036, 28188, 29212, 36381
    dw 35356, 29220, 38429, 30243, 41511, 42537, 41512, 42537, 42538, 44584, 43562, 45611
    dw 35364, 37412, 46637, 37422, 48685, 47662, 48687, 40495, 47662, 40487, 42535, 45610
    dw 45612, 49712, 50732, 49712, 50737, 51761, 49701, 51762, 38437, 38438, 28188, 29211
    dw 30238, 36381, 52787, 52788, 54837, 56886, 55861, 54835, 52788, 53815, 56886, 54838
    dw 55863, 58936, 59962, 60986, 57912, 55867, 57909, 54838, 55863, 56893, 64060, 63037
    dw 64062, 64062, 63037, 575, 2625, 4675, 577, 65088, 5702, 5701, 4677, 575
    dw 576, 7751, 7752, 10825, 9803, 5701, 6725, 8775, 12872, 7757, 14921, 10826
    dw 9802, 12877, 12876, 13903, 15952, 12876, 16976, 12876, 16977, 12876, 18001, 19020
    dw 18003, 19026, 20051, 19026, 21074, 20053, 21076, 22102, 24148, 23126, 25175, 23126
    dw 25176, 23097, 26200, 58937, 59993, 58938, 64061, 27226, 63037, 28250, 63068, 30299
    dw 29276, 30301, 29278, 32349, 31326, 33375, 31329, 35424, 35426, 34403, 37474, 36451
    dw 37476, 36453, 37476, 38501, 37476, 38501, 10852, 38478, 14926, 10826, 14922, 40550
    dw 42600, 43622, 46700, 40550, 39530, 46696, 41577, 43627, 44651, 43627, 44650, 48748
    dw 46703, 45676, 45677, 47723, 44656, 47726, 45676, 45679, 50801, 53875, 52852, 50802
    dw 52851, 53875, 53876, 54900, 51829, 51825, 54898, 50807, 57977, 58998, 50801, 55926
    dw 50801, 55921, 54902, 61045, 60026, 61051, 60026, 60028, 61051, 47728, 49776, 64125
    dw 64126, 49791, 64126, 65151, 638, 65151, 640, 65153, 2688, 1665, 2690, 1665
    dw 1667, 4738, 3715, 5764, 3718, 5765, 6791, 8837, 7815, 56968, 7815, 56951
    dw 7800, 57975, 61052, 9865, 61051, 9865, 10875, 9867, 12938, 11915, 13964, 11918
    dw 13965, 14991, 17037, 16017, 18065, 19088, 18067, 19090, 20115, 21138, 20115, 21140
    dw 20115, 21140, 20076, 22164, 22165, 45676, 48789, 45679, 22744, 24793, 24791, 23770
    dw 24791, 23770
tri_p1:
    dw 16416, 16384, 16448, 16480, 16496, 16496, 16464, 16496, 16528, 16512, 16512, 16544
    dw 16560, 16560, 16496, 16576, 16544, 16576, 16544, 16576, 16560, 16560, 16496, 16560
    dw 16496, 16496, 16544, 16544, 16576, 16544, 16576, 16576, 16592, 16592, 16592, 16608
    dw 16608, 16640, 16624, 16656, 16656, 16672, 16672, 16608, 16624, 16608, 16688, 16608
    dw 16640, 16624, 16672, 16672, 16592, 16608, 16640, 16624, 16656, 16672, 16560, 16608
    dw 16608, 16544, 16544, 16576, 16576, 16688, 16688, 16736, 16736, 16784, 16768, 16688
    dw 16688, 16704, 16800, 16704, 16800, 16816, 16800, 16816, 16800, 16816, 16832, 16816
    dw 16832, 16816, 16832, 16720, 16720, 16848, 16720, 16720, 16848, 16720, 16720, 16864
    dw 16720, 16720, 16864, 16720, 16720, 16864, 16720, 16736, 16736, 16752, 16880, 16752
    dw 16752, 16880, 16752, 16752, 16880, 16896, 16896, 16880, 16896, 16896, 16912, 16896
    dw 16896, 16912, 16896, 16912, 16896, 16928, 16896, 16928, 16768, 16768, 16944, 16768
    dw 16944, 16944, 16688, 16608, 16640, 16736, 16736, 16736, 16752, 16656, 16656, 16784
    dw 16656, 16784, 16784, 16992, 16960, 16976, 16960, 16960, 17088, 17120, 17152, 17168
    dw 17168, 17168, 17168, 17184, 17168, 17184, 17168, 17184, 17200, 17184, 17184, 17216
    dw 17184, 17216, 17184, 17184, 17216, 17120, 17120, 17232, 17088, 17248, 17056, 17136
    dw 17152, 17136, 17136, 17136, 17136, 17152, 17136, 17152, 17136, 17136, 17136, 17296
    dw 17296, 17280, 17280, 17296, 17280, 17296, 17280, 17296, 17280, 17280, 17136, 17136
    dw 17152, 17136, 17312, 17248, 17360, 17328, 17344, 17328, 17344, 17376, 17392, 17376
    dw 17392, 17376, 17392, 17409, 17409, 17441, 17441, 17457, 17473, 17505, 17521, 17473
    dw 17505, 17489, 17521, 17521, 17537, 17537, 17441, 17441, 17553, 17553, 17569, 17569
    dw 17537, 17584, 17361, 17584, 17361, 17361, 17360, 17441, 17441, 17601, 17441, 17601
    dw 17441, 17553, 17553, 17489, 17489, 17617, 17473, 17457, 17473, 17489, 17489, 17617
    dw 17633, 17665, 17665, 17617, 17473, 17505, 17505, 17665, 17633, 17617, 17697, 17633
    dw 17681, 17633, 17648, 17617, 17617, 17633, 17633, 17633, 17648, 17057, 17649, 17665
    dw 17649, 17617, 17617, 17633, 17633, 17649, 17649, 17665, 17649, 17649, 17665, 17665
    dw 17457, 17473, 17489, 17489, 17729, 17713, 17745, 17761, 17745, 17761, 17777, 17777
    dw 17793, 17777, 17809, 17825, 17841, 17825, 17857, 17825, 17857, 17873, 17937, 17937
    dw 17985, 17985, 18001, 18017, 18065, 18017, 18097, 18065, 18001, 18017, 18033, 17905
    dw 18001, 18017, 17985, 17937, 17937, 17953, 18113, 17729, 17729, 17873, 17873, 18129
    dw 18145, 18129, 18129, 18161, 18097, 18081, 18049, 18161, 18081, 18097, 18225, 18209
    dw 18177, 18193, 18225, 18225, 18209, 18209, 18177, 18177, 18193, 18241, 18257, 18289
    dw 18241, 18337, 18241, 18321, 18369, 18369, 18353, 18369, 18353, 18353, 18353, 18241
    dw 18241, 18257, 18257, 18225, 18225, 18209, 18209, 18209, 18177, 18177, 18193, 18193
    dw 18049, 18161, 18081, 18097, 18401, 18401, 18385, 18450, 18418, 18418, 18449, 18402
    dw 18449, 18449, 18498, 18498, 18466, 18498, 18466, 18498, 18514, 18530, 18434, 18546
    dw 18546, 18546, 18594, 18546, 18562, 18594, 18578, 18434, 18562, 18642, 18626, 18594
    dw 18594, 18674, 18674, 18434, 18434, 18450, 18434, 18450, 18450, 18642, 18658, 18482
    dw 18498, 18498, 18530, 18530, 18514, 18530, 18514, 18530, 18674, 18530, 18674, 18674
    dw 18594, 18578, 18690, 18706, 18546, 18594, 18578, 18706, 18722, 18722, 18738, 18738
    dw 18786, 18690, 18706, 18834, 18834, 18866, 18850, 18866, 18882, 18866, 18882, 18866
    dw 18882, 18866, 18866, 18802, 18834, 18690, 18690, 18834, 18834, 18722, 18722, 18738
    dw 18914, 18738, 18850, 18738, 18850, 18866, 18866, 18930, 18930, 18946, 18930, 18946
    dw 18946, 18690, 18690, 18690, 18706, 18690, 18706, 18706, 18546, 18562, 18610, 18610
    dw 18626, 18626, 18594, 18578, 18978, 18962, 18994, 19010, 18994, 19010, 19026, 19026
    dw 19042, 19026, 19058, 19074, 19090, 19074, 19106, 19074, 19106, 19122, 19186, 19186
    dw 19234, 19234, 19250, 19266, 19314, 19266, 19346, 19314, 19250, 19266, 19282, 19154
    dw 19250, 19266, 19234, 19186, 19186, 19202, 19362, 18978, 18978, 19122, 19122, 19378
    dw 19394, 19378, 19378, 19410, 19346, 19330, 19298, 19410, 19330, 19346, 19475, 19459
    dw 19426, 19442, 19474, 19474, 19459, 19459, 19426, 19426, 19442, 19491, 19507, 19539
    dw 19491, 19587, 19491, 19571, 19619, 19619, 19603, 19619, 19603, 19603, 19603, 19491
    dw 19491, 19507, 19507, 19619, 19619, 19459, 19459, 19459, 19427, 19426, 19442, 19442
    dw 19298, 19410, 19330, 19346, 19667, 19667, 19683, 19683, 19731, 19651, 19763, 19699
    dw 19715, 19811, 3475, 3475, 19907, 19891, 19939, 19939, 19955, 19971, 19971, 19955
    dw 19971, 19955, 19971, 19891, 19891, 20003, 19987, 20003, 20051, 20067, 20083, 20067
    dw 20099, 20099, 20083, 20067, 20067, 20147, 20099, 20099, 20099, 19939, 19939, 20179
    dw 20179, 20195, 20195, 20195, 20131, 20211, 20131, 20227, 20227, 20243, 20259, 20003
    dw 20259, 20003, 20003, 20003, 20003, 20275, 20003, 20003, 20275, 20163, 20291, 20163
    dw 20307, 20163, 20307, 20163, 20307, 20307, 20339, 20339, 20323, 20019, 20019, 20035
    dw 20035, 20339, 20339, 20403, 20019, 20019, 20035, 20451, 20435, 20468, 20468, 20483
    dw 20435, 20451, 20451, 20499, 20499, 20356, 20499, 20356, 20499, 20356, 20515, 20532
    dw 20516, 20532, 20532, 20516, 20532, 20548, 20532, 20564, 20532, 20532, 20532, 20532
    dw 20596, 20612, 20612, 20612, 20612, 20612, 20628, 20644, 20643, 20404, 20643, 20404
    dw 20403, 20435, 20435, 20435, 20419, 20675, 20420, 20419, 20339, 20067, 20147, 20147
    dw 20371, 20371, 20019, 20035, 20724, 20756, 20788, 20788, 20788, 20820, 20820, 20804
    dw 20820, 20804, 20820, 20804, 20836, 20804, 20804, 20836, 20852, 20868, 20852, 20852
    dw 20868, 20756, 20756, 20884, 20724, 20900, 20692, 20932, 20948, 20932, 20932, 20932
    dw 20932, 20948, 20932, 20948, 20932, 20932, 20980, 20980, 20980, 20964, 20964, 20980
    dw 20964, 20980, 20996, 20980, 20964, 20964, 21028, 21028, 20788, 20772, 21044, 20900
    dw 21108, 21140, 21172, 21172, 21172, 21172, 21172, 21188, 21204, 21188, 21204, 21188
    dw 21220, 21188, 21188, 21236, 21188, 21252, 21188, 21188, 21252, 21140, 21140, 21268
    dw 21108, 21284, 21076, 21156, 21316, 21156, 21156, 21156, 21156, 21316, 21156, 21332
    dw 21156, 21156, 21364, 21396, 21396, 21380, 21380, 21364, 21380, 21364, 21380, 21364
    dw 21332, 21332, 21364, 21364, 21316, 21156, 20708, 21284, 21460, 21476, 21460, 21508
    dw 21557, 21573, 21605, 21637, 21477, 21572, 21493, 21493, 21509, 21589, 21605, 21685
    dw 21685, 21701, 21701, 21685, 21717, 21717, 21717, 21717, 21781, 21797, 21797, 21813
    dw 21813, 21797, 21813, 21797, 21797, 21749, 21749, 21717, 21749, 21813, 21813, 21845
    dw 21765, 21765, 21781, 21845, 21845, 21845, 21829, 21877, 21877, 21877, 21877, 21909
    dw 21877, 21877, 21909, 21877, 21909, 21909, 21765, 21781, 21925, 21861, 21861, 21925
    dw 21861, 21925, 21957, 22005, 22005, 21989, 21989, 22005, 21989, 22005, 22005, 21925
    dw 21925, 21941, 22037, 21941, 22037, 22021, 22021, 21973, 21957, 22053, 22053, 22053
    dw 22117, 22085, 22133, 22085, 22085, 22085, 22149, 22085, 22149, 22165, 22165, 22181
    dw 22165, 22197, 22165, 22165, 22213, 22165, 22053, 22053, 21957, 21973, 22261, 22229
    dw 21925, 21925, 21925, 22277, 21861, 22277, 21861, 21861, 22325, 22309, 22309, 22341
    dw 22341, 22357, 22389, 22309, 22405, 22309, 22309, 22309, 22341, 22309, 22341, 22421
    dw 22389, 22421, 22389, 22389, 22421, 22389, 22453, 22437, 22485, 22485, 22421, 22389
    dw 22517, 22501, 22421, 22421, 22389, 22389, 22389, 22374, 22533, 22566, 22566, 22582
    dw 22614, 22614, 22630, 22630, 22678, 22662, 22678, 22662, 22662, 22694, 22710, 22694
    dw 22742, 22742, 22550, 22758, 22774, 22790, 22774, 22774, 22646, 22774, 22774, 22806
    dw 22806, 22726, 22806, 22822, 22806, 22806, 22838, 22582, 22630, 22630, 22565, 22470
    dw 22581, 22502, 22870, 22854, 22902, 22902, 22918, 22902, 22902, 22934, 22950, 22934
    dw 22950, 22934, 22934, 22982, 22966, 22982, 22998, 23030, 23062, 23030, 23094, 23094
    dw 23062, 23078, 23078, 23126, 23094, 23094, 23094, 23062, 23062, 22918, 22918, 23126
    dw 23126, 23126, 23158, 23174, 23158, 23174, 23174, 23158, 23190, 22982, 23190, 22982
    dw 22982, 22982, 22982, 23206, 22982, 22982, 23222, 22982, 23238, 22982, 23238, 22982
    dw 23238, 22982, 23094, 23094, 23046, 23046, 23030, 22998, 22998, 23014, 23014, 23270
    dw 23270, 23302, 22998, 22998, 23254, 23382, 23350, 23366, 23366, 23414, 23366, 23382
    dw 23382, 23270, 23270, 23430, 23446, 23430, 23446, 23430, 23462, 23430, 23462, 23430
    dw 23430, 23478, 23430, 23494, 23430, 23494, 23430, 23430, 23430, 23430, 23510, 23526
    dw 23526, 23526, 23527, 23527, 23558, 23527, 23527, 23574, 23590, 23303, 23302, 23350
    dw 23350, 23350, 23254, 23350, 23254, 23254, 23046, 23030, 23318, 23319, 23606, 23606
    dw 22998, 23014, 23655, 23655, 23655, 23671, 23655, 23687, 23687, 23687, 23687, 23751
    dw 23735, 23783, 23783, 23783, 23751, 23783, 23751, 23751, 23735, 23735, 23735, 23735
    dw 23799, 23815, 23799, 23815, 23815, 23815, 23799, 23799, 23831, 23847, 23831, 23847
    dw 23831, 23847, 23831, 23815, 23815, 23815, 23879, 23815, 23879, 23815, 23815, 23879
    dw 23879, 23783, 23783, 23783, 23767, 23767, 23895, 23943, 23959, 23959, 23959, 23927
    dw 23959, 24007, 23991, 23991, 24023, 24039, 24039, 24055, 24087, 24071, 24087, 24071
    dw 24087, 23975, 23975, 24055, 23975, 24055, 24055, 24071, 24071, 23991, 24071, 23991
    dw 23991, 24103, 24103, 24103, 24119, 24119, 24151, 24135, 24183, 24183, 24167, 24167
    dw 24119, 24247, 24215, 24263, 24215, 24231, 24247, 24295, 24295, 24103, 24119, 24151
    dw 24247, 24183, 24167, 24039, 24119, 24119, 24183, 24183, 24087, 24087, 24327, 24327
    dw 24359, 24359, 24439, 24407, 24455, 24455, 24311, 24455, 24311, 24471, 24487, 24503
    dw 24519, 24503, 24519, 24535, 24551, 24567, 24552, 24583, 24552, 24552, 24599, 24616
    dw 24616, 24632, 24648, 24648, 24664, 24648, 24648, 24664, 24648, 24647, 24360, 24343
    dw 24359, 24359, 24376, 24695, 24376, 24376, 24711, 24376, 24376, 24711, 24376, 24376
    dw 24727, 24744, 24744, 24760, 24744, 24744, 24760, 24744, 24776, 24424, 24775, 24424
    dw 24791, 24424, 24424, 24807, 24423, 24439, 24439, 24263, 24215, 24231, 24359, 24359
    dw 24359, 24391, 24279, 24280, 24823, 24280, 24823, 24823, 24872, 24856, 24856, 24888
    dw 24888, 24904, 24952, 24936, 24968, 24856, 24856, 24856, 24968, 24856, 24888, 24936
    dw 25000, 24936, 24952, 24952, 24984, 25000, 25016, 25000, 25080, 25080, 24984, 25000
    dw 25080, 25096, 24936, 24936, 24952, 24952, 24952, 24920, 25112, 25128, 25128, 25048
    dw 25128, 25128, 25192, 25192, 25240, 25224, 25240, 25224, 25224, 25256, 25272, 25256
    dw 25304, 25304, 25160, 25304, 25320, 25336, 25320, 25320, 25208, 25320, 25320, 25352
    dw 25352, 25288, 25352, 25368, 25352, 25352, 25384, 25176, 25384, 25384, 25016, 25032
    dw 25048, 25064, 25416, 25416, 25448, 25464, 25432, 25448, 25448, 25448, 25464, 25448
    dw 25448, 25512, 25480, 25480, 25528, 25480, 25448, 25448, 25544, 25544, 25560, 25576
    dw 25560, 25560, 25576, 25609, 25609, 25689, 25593, 25608, 25689, 25705, 25705, 25721
    dw 25721, 25609, 25609, 25689, 25689, 25785, 25785, 25721, 25721, 25801, 25769, 25753
    dw 25753, 25817, 25817, 25801, 25801, 25865, 25801, 25865, 25801, 25881, 25801, 25881
    dw 25897, 25913, 25897, 25913, 25913, 25929, 25945, 25929, 25961, 25977, 25961, 25993
    dw 25961, 25993, 25497, 26008, 25496, 25512, 26024, 25561, 26024, 25561, 26040, 26057
    dw 26073, 26057, 26073, 26089, 26105, 26089, 26121, 26137, 26137, 26153, 26169, 26185
    dw 26169, 26185, 26201, 26185, 26201, 26185, 26201, 25769, 25769, 25833, 25833, 26217
    dw 26249, 26297, 26249, 26281, 26217, 26249, 26329, 26345, 26345, 26281, 26297, 26313
    dw 26313, 26329, 26329, 26345, 26345, 26377, 26361, 26361, 26409, 26425, 26441, 26425
    dw 26409, 26409, 26409, 26409, 26457, 26473, 26473, 26505, 26393, 26393, 26473, 26393
    dw 26473, 26473, 26537, 26537, 26553, 26537, 26553, 26553, 26569, 26585, 26585, 26377
    dw 26377, 26601, 26617, 26601, 26618, 26633, 26618, 26633, 26650, 26666, 26650, 26666
    dw 26666, 26682, 26698, 26682, 26714, 26730, 26714, 26746, 26762, 26745, 26490, 26745
    dw 26490, 26505, 26777, 26554, 26777, 26554, 26778, 26794, 26810, 26826, 26810, 26842
    dw 26858, 26842, 26874, 26890, 26890, 26906, 26922, 26938, 26922, 26938, 26954, 26938
    dw 26954, 26938, 26954, 26314, 26314, 26969, 26313, 26361, 10618, 10611, 10650, 10643
    dw 3475, 3475

align 2, db 0
bss_start:
absolute bss_start
player_x:           resw 1
player_y:           resw 1
view_angle:         resw 1
held_keys:          resb 1
                    resb 1
pending_mouse_dx:   resw 1
frame_start_pit:    resw 1
mouse_phase:        resb 1
mouse_packet_0:     resb 1
mouse_packet_1:     resb 1
keyboard_extended:  resb 1
kbc_status_byte:    resb 1
kbc_data_byte:      resb 1
ray_x:              resw 1
ray_y:              resw 1
ray_distance_q6:    resw 1
ray_hit_q6:         resw 1
ray_map_x:          resw 1
ray_map_y:          resw 1
ray_step_x:         resw 1
ray_step_y:         resw 1
ray_delta_x:        resw 1
ray_delta_y:        resw 1
ray_side_x:         resw 1
ray_side_y:         resw 1
ray_side:           resb 1
ray_hit:            resb 1
projected_x0:       resw 1
projected_y0:       resw 1
projected_z0:       resw 1
projected_x1:       resw 1
projected_y1:       resw 1
projected_z1:       resw 1
projected_x2:       resw 1
projected_y2:       resw 1
projected_z2:       resw 1
model_sin:          resw 1
model_cos:          resw 1
span_y:             resw 1
span_left_q3:       resw 1
span_right_q3:      resw 1
span_left_z:        resw 1
span_right_z:       resw 1
span_color:         resb 1