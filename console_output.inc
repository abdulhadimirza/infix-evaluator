; -----------------------------------------
; Print postfix expression
; Input: DI = address immediately after output
; Uses DOS function 02h for character output
; -----------------------------------------
print_postfix proc
    mov ax, di                ; AX = address after postfix output
    lea bx, postfix           ; BX = beginning of postfix buffer
    sub ax, bx                ; AX = number of output characters
    mov [postfix_len], al     ; Store lower byte of length

    mov ah, 09h               ; Display postfix label
    lea dx, postfix_prompt ; DX = address of the postfix label
    int 21h                ; Print the label

    lea si, postfix           ; SI = beginning of postfix output

    mov cl, [postfix_len]     ; CL = output length
    mov ch, 0                 ; Clear CH because LOOP uses full CX

    jcxz postfix_printed      ; Skip loop if CX == 0

    print_loop:
        mov dl, [si]          ; Load next character into DL
        mov ah, 02h           ; DOS function 02h prints DL
        int 21h                  ; Print the current postfix character

        inc si                ; Advance output read pointer
        loop print_loop       ; Decrement CX, repeat while CX != 0

    postfix_printed:
        ret                   ; Return after printing all postfix characters

print_postfix endp


; -----------------------------------------
; Wait for keypress and terminate program
; -----------------------------------------
exit_program proc
    mov ah, 09h               ; Display exit prompt
    lea dx, exit_prompt     ; DX = address of the final pause message
    int 21h                  ; Display the message

    mov ah, 01h               ; Wait for keyboard input (with echo)
    int 21h                  ; Wait for one keypress

    mov ah, 4Ch               ; DOS terminate process function
    int 21h                   ; Terminate and return control to DOS

exit_program endp
