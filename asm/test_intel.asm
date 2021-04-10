bits 64

mov eax, 10
mov rax, rcx
mov eax, ecx
mov ax, cx
mov al, cl
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
call .test
.test:
call rax
ret
