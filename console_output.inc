; -----------------------------------------
; Print postfix expression
; Input: postfix and postfix_len contain the converted expression
; Output: Postfix label and all converted output characters are printed
; Preserves: None
; Clobbers: AX, CX, DX, SI, flags
; Conventions: Reads exactly postfix_len characters from the postfix buffer
; Error behavior: An empty postfix buffer prints only its label
; Uses DOS function 02h for character output
; -----------------------------------------
print_postfix proc ; Begin the postfix output procedure
    mov ah, 09h               ; Display postfix label
    lea dx, postfix_prompt ; DX = address of the postfix label
    int 21h                ; Print the label

    lea si, postfix           ; SI = beginning of postfix output

    mov cl, [postfix_len]     ; CL = output length
    mov ch, 0                 ; Clear CH because LOOP uses full CX

    jcxz postfix_printed      ; Skip loop if CX == 0

    print_loop: ; Emit each character while the output count remains
        mov dl, [si]          ; Load next character into DL
        mov ah, 02h           ; DOS function 02h prints DL
        int 21h                  ; Print the current postfix character

        inc si                ; Advance output read pointer
        loop print_loop       ; Decrement CX, repeat while CX != 0

    postfix_printed: ; Return after the output loop or empty-buffer check
        ret                   ; Return after printing all postfix characters

print_postfix endp ; End the postfix output procedure


; -----------------------------------------
; Wait for keypress and terminate program
; Input: None
; Output: Program terminates after one echoed keypress
; Preserves: None
; Clobbers: AX, DX, flags
; Conventions: DOS function 4Ch exits the process
; Error behavior: None
; -----------------------------------------
exit_program proc ; Begin the pause-and-terminate procedure
    mov ah, 09h               ; Display exit prompt
    lea dx, exit_prompt     ; DX = address of the final pause message
    int 21h                  ; Display the message

    mov ah, 01h               ; Wait for keyboard input (with echo)
    int 21h                  ; Wait for one keypress

    mov ah, 4Ch               ; DOS terminate process function
    int 21h                   ; Terminate and return control to DOS

exit_program endp ; End the pause-and-terminate procedure


; -----------------------------------------
; Print the signed final result in decimal
; Input:  result = signed 16-bit result
; Output: Result label and decimal digits are written to standard output
; Preserves: SI, DI, BP
; Clobbers: AX, flags
; Conventions: Emits a leading minus sign for negative values; zero prints as 0
; Error behavior: None; every 16-bit value has a printable decimal representation
; -----------------------------------------
print_signed_result proc ; Begin signed decimal result formatting
    push bx                         ; Preserve BX while formatting decimal digits
    push cx                         ; Preserve CX while counting saved decimal digits
    push dx                         ; Preserve DX while dividing the unsigned magnitude
    mov ah, 09h                    ; Select DOS terminated-string output
    lea dx, result_prompt          ; DX = address of the result label
    int 21h                        ; Print the label and begin a new result line

    mov ax, result                 ; AX = signed result to format
    cmp ax, 0                      ; Determine whether the result is negative
    jge result_magnitude_ready     ; Skip sign handling for nonnegative values
    mov dl, '-'                    ; DL = visible minus sign
    mov ah, 02h                    ; Select DOS single-character output
    int 21h                        ; Print the negative sign
    mov ax, result                  ; Reload the signed value after DOS output
    neg ax                          ; Convert to unsigned magnitude, including 32768

    result_magnitude_ready: ; Begin decimal conversion with the unsigned magnitude in AX
        xor cx, cx                      ; CX = number of decimal digits saved on CPU stack
        mov bx, 10                      ; BX = decimal divisor

    result_digit_loop: ; Save decimal digits from least significant to most significant
        xor dx, dx                      ; Clear high dividend word before unsigned division
        div bx                          ; AX = quotient, DX = next base-10 remainder
        push dx                         ; Save the least-significant digit for reverse output
        inc cx                          ; Count this saved digit
        cmp ax, 0                       ; Check whether more quotient digits remain
        jne result_digit_loop           ; Continue until the quotient is zero

    result_print_loop: ; Restore and print saved digits in decimal order
        pop dx                          ; Restore the next most-significant digit
        add dl, '0'                     ; Convert numeric remainder to an ASCII character
        mov ah, 02h                     ; Select DOS single-character output
        int 21h                         ; Print the current decimal digit
        loop result_print_loop          ; Continue until every saved digit is printed

        pop dx                          ; Restore the caller's DX register
        pop cx                          ; Restore the caller's CX register
        pop bx                          ; Restore the caller's BX register
        ret                             ; Return after printing the signed decimal result
print_signed_result endp ; End signed decimal result formatting
