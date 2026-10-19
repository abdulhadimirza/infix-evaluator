; -----------------------------------------
; Convert infix expression to postfix
; Using the shunting-yard algorithm
; Input:  expression and expr_len contain the DOS buffered input
; Output: postfix and postfix_len are written; DI points after the final character
; Preserves: None
; Clobbers: AX, BX, DX, SI, DI, flags
; Conventions: operator_stack and stack_top hold operators; postfix_len records emitted bytes
;              CF from stack helpers signals failure
; Error behavior: Sets error_flag and returns when syntax or operator-stack validation fails
;
; SI = current input character address
; DI = next free postfix output address
; BX = address immediately after input
; AL = current character/operator
; AH = operator precedence
; -----------------------------------------
convert_to_postfix proc ; Begin infix validation and shunting-yard conversion
    lea si, expression        ; Load address of input buffer
    add si, 2                 ; Skip capacity and length bytes

    lea di, postfix           ; Point DI to beginning of output buffer

    xor ax, ax                ; Clear AH before loading 8-bit length
    mov al, [expr_len]        ; AX = number of input characters
    mov bx, si                ; BX = beginning of actual input
    add bx, ax                ; BX = address immediately after input

    cmp si, bx                ; Check for empty input
    je conversion_error      ; Empty input is invalid

    mov expect_operand, 1     ; An expression must begin with an operand

    ; -----------------------------------------
    ; Process each input character
    ; -----------------------------------------
    convert_loop: ; Process input until its end address is reached
        cmp si, bx            ; Have we reached the end of input?
        jae flush_stack       ; JAE = unsigned above or equal

        mov al, [si]          ; Load current character

        cmp al, '0'           ; Check whether character is a digit
        jb check_symbols      ; Below '0' means not a digit
        cmp al, '9'           ; Compare with the highest decimal digit
        jbe output_operand    ; Between '0' and '9' inclusive

    check_symbols: ; Check operators, parentheses, spaces, and unsupported characters
        cmp al, '+'           ; Check supported operators
        je handle_operator   ; Route to precedence handling
        cmp al, '-'           ; Check subtraction
        je handle_operator   ; Route to precedence handling
        cmp al, '*'           ; Check multiplication
        je handle_operator   ; Route to precedence handling
        cmp al, '/'           ; Check division
        je handle_operator   ; Route to precedence handling
        cmp al, '^'           ; Check exponentiation
        je handle_operator   ; Route to precedence handling

        cmp al, '('           ; Opening parenthesis
        je handle_left_paren ; Push an opening grouping marker

        cmp al, ')'           ; Closing parenthesis
        je handle_right_paren ; Pop operators through the matching '('

        cmp al, ' '           ; Ignore spaces between tokens
        je next_character    ; Ignore this space and continue scanning

        jmp conversion_error  ; Unsupported character


    ; -----------------------------------------
    ; Copy a complete numeric operand
    ; Example: "123" is one operand
    ; -----------------------------------------
    output_operand: ; Validate and copy a complete decimal operand
        cmp expect_operand, 1 ; Reject adjacent numbers or a number after ')'
        jne conversion_error ; Reject a number after another operand

    copy_number: ; Copy consecutive digits from the current operand
        cmp si, bx            ; Check input boundary before reading
        jae number_done      ; Stop at end of entered expression

        mov al, [si]          ; Read current character

        cmp al, '0'           ; Stop if character is not a digit
        jb number_done       ; Stop if current character is below '0'
        cmp al, '9'           ; Compare with the highest decimal digit
        ja number_done       ; Stop when this character is not a digit

        mov [di], al          ; Copy digit to postfix buffer
        inc di                ; Move output pointer forward
        inc si                ; Move input pointer forward

        jmp copy_number       ; Continue reading consecutive digits

    number_done: ; Separate the completed operand from the next postfix token
        mov byte ptr [di], ' ' ; Separate postfix tokens
        inc di                ; Advance to next output position
        mov expect_operand, 0 ; A number was read; next expect operator or ')'

        jmp convert_loop      ; SI already points to next non-digit


    ; -----------------------------------------
    ; Handle arithmetic operator
    ; Pop higher-precedence operators first
    ; Pop equal precedence if left-associative
    ; -----------------------------------------
    handle_operator: ; Validate an operator and compare its precedence with the stack top
        cmp expect_operand, 1 ; Reject leading or repeated binary operators
        jne operator_is_valid ; Continue if an operand precedes this operator
        jmp conversion_error ; Record invalid operator placement

    operator_is_valid: ; Save the valid incoming operator and its precedence
        mov expect_operand, 1 ; An operator must be followed by an operand
        call get_precedence   ; AL = operator, AH = precedence
        push ax               ; Save incoming operator and precedence

    operator_loop: ; Pop higher-precedence or left-associative equal-precedence operators
        cmp stack_top, 0      ; Check if operator stack is empty
        je push_current_operator ; No stacked operator needs to be popped

        push bx               ; Preserve input end address
        mov bx, stack_top     ; BX = next free stack index
        dec bx                ; BX = index of top operator
        mov al, [operator_stack + bx] ; Peek without removing
        pop bx                ; Restore input end address

        call get_precedence   ; AH = precedence of top operator

        pop dx                ; DL = incoming operator, DH = precedence
        push dx               ; Preserve them for another iteration

        cmp ah, dh            ; Compare top and incoming precedence
        jb push_current_operator ; Top precedence is lower
        ja pop_top_operator   ; Top precedence is higher

        cmp dl, '^'           ; '^' is right-associative
        je push_current_operator ; Equal '^' precedence: do not pop

        ; Other operators are left-associative, so pop on equality

    pop_top_operator: ; Emit the stacked operator and continue precedence comparison
        call pop_operator     ; Remove top operator, return it in AL
        jc operator_conversion_error ; CF = 1 means underflow

        mov [di], al          ; Append popped operator to postfix
        inc di                ; Advance to the next output position
        mov byte ptr [di], ' ' ; Add token separator
        inc di                ; Advance beyond emitted operator

        jmp operator_loop     ; Check remaining stacked operators

    push_current_operator: ; Push the incoming operator after all required pops
        pop ax                ; Restore original operator into AL
        call push_operator    ; Push operator onto custom stack
        jc conversion_error   ; CF = 1 means stack overflow

        jmp next_character   ; Continue scanning after pushing operator


    ; -----------------------------------------
    ; Handle opening parenthesis '('
    ; -----------------------------------------
    handle_left_paren: ; Push a grouping boundary when an operand is expected
        cmp expect_operand, 1 ; Reject implicit multiplication, such as 2(3)
        jne conversion_error ; Reject '(' when an operand is not expected
        call push_operator    ; Push '(' as precedence boundary
        jc conversion_error ; Reject operator-stack overflow

        jmp next_character   ; Continue scanning inside the group


    ; -----------------------------------------
    ; Handle closing parenthesis ')'
    ; Pop until matching '(' is found
    ; -----------------------------------------
    handle_right_paren: ; Pop operators until the matching opening parenthesis
        cmp expect_operand, 1 ; Reject empty parentheses or a missing operand
        je conversion_error  ; Reject ')' when no operand precedes it

    right_paren_loop: ; Emit each operator before the matching opening parenthesis
        call pop_operator     ; Pop next operator into AL
        jc conversion_error   ; Empty stack means unmatched ')'

        cmp al, '('           ; Found matching opening parenthesis?
        je right_paren_matched ; Matching '(' ends the pop loop

        mov [di], al          ; Append operator to postfix
        inc di                ; Advance to the next output position
        mov byte ptr [di], ' ' ; Append separator
        inc di                ; Advance beyond emitted operator

        jmp right_paren_loop  ; Continue until '(' is found

    right_paren_matched: ; Mark the closed group as a completed operand
        mov expect_operand, 0 ; A closing parenthesis completes an operand
        jmp next_character   ; Continue after closing the group


    ; -----------------------------------------
    ; Advance input pointer
    ; -----------------------------------------
    next_character: ; Advance past a single nonnumeric input character
        inc si                ; Move to next input character
        jmp convert_loop     ; Process the next input character


    ; -----------------------------------------
    ; Flush remaining stacked operators
    ; -----------------------------------------
    flush_stack: ; Emit remaining operators after validating the final operand
        cmp expect_operand, 1 ; Reject an expression ending with an operator
        je conversion_error  ; Reject expression ending with an operator
        cmp stack_top, 0      ; Check whether stack is empty
        je conversion_done    ; Conversion finished when empty

        call pop_operator     ; Remove remaining operator
        jc conversion_error ; Reject an unexpected empty operator stack

        cmp al, '('           ; Unmatched opening parenthesis?
        je conversion_error  ; Reject an opening parenthesis that was never closed

        mov [di], al          ; Append operator to postfix
        inc di                ; Advance beyond emitted operator
        mov byte ptr [di], ' ' ; Append separator
        inc di                ; Advance beyond the output separator

        jmp flush_stack       ; Continue until stack becomes empty


    ; -----------------------------------------
    ; Handle conversion errors
    ; -----------------------------------------
    operator_conversion_error: ; Discard the saved incoming operator before common error handling
        pop dx                ; Discard saved incoming operator
        jmp conversion_error ; Route through the common error handler

    conversion_error: ; Mark the expression invalid for the main procedure
        mov error_flag, 1     ; Inform main procedure of failure
        jmp conversion_done  ; Return after setting the error flag

    conversion_done: ; Return after success or after recording a conversion error
        mov ax, di                ; AX = address immediately after postfix output
        lea bx, postfix           ; BX = beginning of postfix buffer
        sub ax, bx                ; AX = number of emitted postfix characters
        mov [postfix_len], al     ; Save the output length for display and evaluation
        ret                   ; DI points immediately after output

convert_to_postfix endp ; End infix validation and postfix conversion
