bits 64
mov eax, 10
mov bx, 0x5
mov cl, 0b11
mov rbx, 0x60000000000
mov ah, 0x1
push rbx
pop rax
inc eax
inc ebx
dec ax
dec rcx
inc al
dec ah
dec eax
ret
