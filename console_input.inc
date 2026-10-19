; -----------------------------------------
; Ask whether to evaluate another expression
; Input: None
; Output: AL = 'Y' or 'N'
; Preserves: None
; Clobbers: AX, DX, flags
; Conventions: Lowercase choices are normalized to uppercase; other keys are rejected
; Error behavior: Re-prompts until Y or N is entered
; -----------------------------------------
ask_to_continue proc ; Begin the continue-choice input procedure
    ask_again: ; Repeat the prompt after an invalid choice
        mov ah, 09h           ; Display the continue prompt
        lea dx, continue_prompt ; DX = continue-prompt address
        int 21h                  ; Display the continue prompt

        mov ah, 01h           ; Read one key with echo
        int 21h                  ; Read the user's Y/N key

        and al, 0DFh          ; Convert lowercase letters to uppercase
        cmp al, 'Y'           ; Accept uppercase Y
        je continue_choice_done ; Return when Y was entered
        cmp al, 'N'           ; Accept uppercase N
        je continue_choice_done ; Return when N was entered

        mov ah, 09h           ; Show a message for any other key
        lea dx, continue_error_prompt ; DX = invalid-choice message
        int 21h                        ; Display the message
        jmp ask_again                  ; Ask again until Y or N is entered

    continue_choice_done: ; Return the accepted Y or N choice
        ret                      ; Return the accepted choice in AL
ask_to_continue endp ; End the continue-choice input procedure


; -----------------------------------------
; Read expression from keyboard
; Input:  None
; Output: expression, expr_len
; Preserves: None
; Clobbers: AX, DX, flags
; Conventions: DOS buffered input stores its character count in expression+1
; Error behavior: Returns the entered line for conversion and validation
; -----------------------------------------
read_expression proc ; Begin the buffered expression input procedure
    mov ah, 09h               ; Display input prompt
    lea dx, input_prompt      ; DX = address of the input prompt
    int 21h                   ; Display the prompt

    mov ah, 0Ah               ; DOS buffered keyboard input
    lea dx, expression        ; DX = address of input buffer
    int 21h                   ; Read the expression into the DOS buffer

    mov al, [expression + 1]  ; Second buffer byte holds input length
    mov [expr_len], al        ; Save length for conversion procedure

    ret                      ; Return after saving the input length
read_expression endp ; End the buffered expression input procedure
