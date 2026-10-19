; -----------------------------------------
; Evaluate the space-separated postfix expression
; Input:  postfix, postfix_len contain the converted expression
; Output: result contains the final signed 16-bit value
; Preserves: CX, SI, DI, BP
; Clobbers: AX, BX, DX, flags
; Conventions: Decimal operands are nonnegative; operators are binary; spaces separate tokens
; Error behavior: Returns CF=1 for malformed tokens, invalid stack state, overflow, zero division, or negative exponent
; -----------------------------------------
evaluate_postfix proc ; Begin token scanning and postfix evaluation
    mov eval_top, 0                  ; Start with an empty evaluation stack
    mov eval_pos, 0                  ; Start scanning at the first postfix character

    evaluate_token_loop: ; Continue until the recorded postfix length is consumed
        mov ax, eval_pos                 ; AX = current postfix character offset
        cmp al, postfix_len              ; Check whether every output character was scanned
        jae evaluate_tokens_done         ; Finish token scanning at the end of postfix output

        xor bx, bx                       ; Clear BX before using the byte offset
        mov bx, eval_pos                 ; BX = current character offset
        mov al, [postfix + bx]           ; AL = current postfix character
        cmp al, ' '                      ; Skip token-separator spaces
        jne evaluate_nonspace_token      ; Dispatch every nonspace character
        inc eval_pos                     ; Advance past the current separator
        jmp evaluate_token_loop          ; Continue scanning the next character

    evaluate_nonspace_token: ; Dispatch a digit operand or a supported operator
        cmp al, '0'                      ; Check whether the token begins with a decimal digit
        jb evaluate_operator_token       ; Characters below '0' may be binary operators
        cmp al, '9'                      ; Compare against the largest decimal digit
        ja evaluate_operator_token       ; Characters above '9' must be recognized operators
        call parse_postfix_operand       ; Parse one complete nonnegative decimal operand
        jc evaluate_failed               ; Reject an operand that overflows signed 16-bit range
        call push_eval_value             ; Push the parsed operand onto the evaluation stack
        jc evaluate_failed               ; Reject a full evaluation stack
        jmp evaluate_token_loop          ; Continue with the next postfix token

    evaluate_operator_token: ; Match the current character to a supported binary operator
        cmp al, '+'                      ; Check addition operator
        je evaluate_operator_valid       ; Dispatch a supported binary operator
        cmp al, '-'                      ; Check subtraction operator
        je evaluate_operator_valid       ; Dispatch a supported binary operator
        cmp al, '*'                      ; Check multiplication operator
        je evaluate_operator_valid       ; Dispatch a supported binary operator
        cmp al, '/'                      ; Check signed division operator
        je evaluate_operator_valid       ; Dispatch a supported binary operator
        cmp al, '^'                      ; Check integer exponentiation operator
        jne evaluate_failed              ; Reject any unsupported postfix character

    evaluate_operator_valid: ; Save and apply the recognized operator
        mov current_operator, al         ; Save the operator before helper procedures use AL
        call apply_postfix_operator      ; Pop two operands, apply the operator, and push its result
        jc evaluate_failed               ; Reject missing operands or invalid arithmetic
        inc eval_pos                     ; Advance past the operator token
        jmp evaluate_token_loop          ; Continue with the next postfix token

    evaluate_tokens_done: ; Require one final value and store it as result
        cmp eval_top, 1                  ; A valid expression must leave exactly one value
        jne evaluate_failed              ; Reject an empty or overfull final evaluation stack
        call pop_eval_value              ; Remove the final value from the evaluation stack
        jc evaluate_failed               ; Reject an inconsistent stack state
        mov result, ax                   ; Save the completed signed result
        clc                              ; Return success in the carry flag
        ret                              ; Return with result populated

    evaluate_failed: ; Report malformed postfix or an arithmetic/stack failure
        stc                              ; Return failure in the carry flag
        ret                              ; Return without displaying a result
evaluate_postfix endp ; End token scanning and postfix evaluation


; -----------------------------------------
; Parse one nonnegative decimal postfix operand
; Input:  eval_pos points to the first digit in postfix
; Output: AX = parsed signed 16-bit operand; eval_pos points to its next token boundary
; Preserves: CX, SI, DI, BP
; Clobbers: AX, BX, DX, flags
; Conventions: Consecutive digits form one operand; values above 32767 are invalid
; Error behavior: Returns CF=1 for decimal overflow or a missing token separator
; -----------------------------------------
parse_postfix_operand proc ; Begin decimal operand parsing
    xor ax, ax                       ; AX = initial decimal accumulator

    parse_operand_digit_loop: ; Accumulate consecutive decimal digits
        mov bx, eval_pos                 ; BX = current digit offset
        cmp bl, postfix_len              ; Check for end of postfix before reading the next character
        jae parse_operand_done           ; Finish a final operand at the end of the buffer
        mov dl, [postfix + bx]           ; DL = current postfix character
        cmp dl, '0'                      ; Check whether the current character is below the digit range
        jb parse_operand_separator       ; Require a space or end of buffer after the operand
        cmp dl, '9'                      ; Check whether the current character is above the digit range
        ja parse_operand_separator       ; Require a space or end of buffer after the operand
        sub dl, '0'                      ; Convert the ASCII digit to its numeric value
        mov eval_digit, dl               ; Preserve the digit while multiplying the accumulator
        mov bx, 10                       ; BX = decimal base
        mul bx                           ; DX:AX = current accumulator multiplied by ten
        cmp dx, 0                        ; Check whether multiplication exceeded 16 bits
        jne parse_operand_failed         ; Reject an accumulator too large for AX
        xor bx, bx                       ; Clear BX before adding the saved digit
        mov bl, eval_digit               ; BX = numeric value of the current digit
        add ax, bx                       ; Append the current decimal digit
        jc parse_operand_failed          ; Reject unsigned carry from decimal accumulation
        test ax, 8000h                   ; Signed operands must not exceed 32767
        jnz parse_operand_failed         ; Reject values outside the nonnegative signed range
        inc eval_pos                     ; Advance to the next postfix character
        jmp parse_operand_digit_loop     ; Continue while consecutive decimal digits remain

    parse_operand_separator: ; Validate that the parsed operand ends at a token boundary
        cmp dl, ' '                      ; Postfix operands must end before a space separator
        jne parse_operand_failed         ; Reject adjacent digits and operator characters as malformed tokens

    parse_operand_done: ; Return the completed operand in AX
        clc                              ; Return the parsed operand successfully
        ret                              ; Return the decimal value in AX

    parse_operand_failed: ; Return carry when the operand exceeds the signed range
        stc                              ; Report decimal operand overflow
        ret                              ; Return failure to the evaluator
parse_postfix_operand endp ; End decimal operand parsing


; -----------------------------------------
; Push one signed value onto the evaluation stack
; Input:  AX = signed 16-bit value
; Output: evaluation_stack contains the value and eval_top advances on success
; Preserves: AX, BX, CX, DX, SI, DI, BP
; Clobbers: flags
; Conventions: eval_top counts occupied words; the next free slot is at index eval_top
; Error behavior: Returns CF=1 when all 100 stack entries are occupied
; -----------------------------------------
push_eval_value proc ; Begin bounded evaluation-stack push
    cmp eval_top, 100                ; Check the fixed evaluation stack capacity
    jae push_eval_failed             ; Reject a value when the stack is full
    push bx                          ; Preserve BX while computing the word offset
    mov bx, eval_top                 ; BX = next free word index
    shl bx, 1                       ; Convert the word index to a byte offset
    mov [evaluation_stack + bx], ax  ; Store the signed value in the next free slot
    inc eval_top                     ; Mark the new stack entry as occupied
    pop bx                           ; Restore the caller's BX register
    clc                              ; Return success in the carry flag
    ret                              ; Return after pushing the value

    push_eval_failed: ; Report evaluation-stack capacity exhaustion
        stc                              ; Return stack overflow in the carry flag
        ret                              ; Return without changing the stack
push_eval_value endp ; End bounded evaluation-stack push


; -----------------------------------------
; Pop one signed value from the evaluation stack
; Input:  evaluation_stack and eval_top describe the current stack
; Output: AX = removed value; eval_top decreases on success
; Preserves: BX, CX, DX, SI, DI, BP
; Clobbers: AX, flags
; Conventions: The top entry is at index eval_top minus one
; Error behavior: Returns CF=1 when the evaluation stack is empty
; -----------------------------------------
pop_eval_value proc ; Begin bounded evaluation-stack pop
    cmp eval_top, 0                  ; Check whether a value is available to pop
    je pop_eval_failed               ; Reject an empty stack
    push bx                          ; Preserve BX while computing the word offset
    dec eval_top                     ; Move the occupied count to the former top index
    mov bx, eval_top                 ; BX = index of the value to remove
    shl bx, 1                       ; Convert the word index to a byte offset
    mov ax, [evaluation_stack + bx]  ; Load the signed value into AX
    pop bx                           ; Restore the caller's BX register
    clc                              ; Return success in the carry flag
    ret                              ; Return the popped value in AX

    pop_eval_failed: ; Report evaluation-stack underflow
        stc                              ; Return stack underflow in the carry flag
        ret                              ; Return without changing the stack
pop_eval_value endp ; End bounded evaluation-stack pop


; -----------------------------------------
; Apply one binary postfix operator
; Input:  current_operator contains the operator; stack contains right then left operand
; Output: The computed signed 16-bit value is pushed onto the evaluation stack
; Preserves: BX, CX, SI, DI, BP
; Clobbers: AX, DX, flags
; Conventions: Operands are popped right-to-left; division truncates toward zero
; Error behavior: Returns CF=1 for underflow, overflow, division by zero, or negative exponent
; -----------------------------------------
apply_postfix_operator proc ; Begin binary operator dispatch and arithmetic
    call pop_eval_value              ; Pop the right operand into AX
    jc apply_operator_failed         ; Reject an operator without a right operand
    mov eval_right, ax               ; Save the right operand for the selected operation
    call pop_eval_value              ; Pop the left operand into AX
    jc apply_operator_failed         ; Reject an operator without a left operand
    mov eval_left, ax                ; Save the left operand for the selected operation

    cmp current_operator, '+'        ; Select signed addition
    je apply_addition                ; Compute left operand plus right operand
    cmp current_operator, '-'        ; Select signed subtraction
    je apply_subtraction             ; Compute left operand minus right operand
    cmp current_operator, '*'        ; Select signed multiplication
    je apply_multiplication           ; Compute left operand times right operand
    cmp current_operator, '/'        ; Select signed division
    je apply_division                ; Compute left operand divided by right operand
    jmp apply_exponentiation         ; The remaining supported operator is '^'

    apply_addition: ; Compute and range-check the signed sum
        mov ax, eval_left                ; AX = left addend
        add ax, eval_right               ; AX = signed sum
        jo apply_operator_failed        ; Reject signed overflow
        jmp apply_operation_result       ; Push the valid sum

    apply_subtraction: ; Compute and range-check the signed difference
        mov ax, eval_left                ; AX = minuend
        sub ax, eval_right               ; AX = signed difference
        jo apply_operator_failed        ; Reject signed overflow
        jmp apply_operation_result       ; Push the valid difference

    apply_multiplication: ; Compute a signed double-word product and check its range
        mov ax, eval_left                ; AX = left factor
        mov dx, eval_right               ; DX = right factor
        imul dx                          ; DX:AX = signed 32-bit product
        call check_product_range         ; Verify the product fits in signed 16-bit range
        jc apply_operator_failed        ; Reject a product outside the result range
        jmp apply_operation_result       ; Push the low word of the valid product

    apply_division: ; Guard signed division and produce a truncated quotient
        cmp eval_right, 0                ; Check the divisor before executing IDIV
        je apply_operator_failed        ; Reject division by zero
        cmp eval_left, 8000h             ; Check the only signed division overflow case
        jne division_is_safe             ; Other signed dividends cannot overflow IDIV here
        cmp eval_right, -1               ; Check whether -32768 is divided by -1
        je apply_operator_failed        ; Reject the quotient 32768 as out of range

    division_is_safe: ; Divide after excluding zero and the signed overflow case
        mov ax, eval_left                ; AX = signed dividend
        cwd                              ; Sign-extend the dividend into DX:AX
        idiv eval_right                  ; AX = quotient truncated toward zero
        jmp apply_operation_result       ; Push the valid quotient

    apply_exponentiation: ; Compute a nonnegative integer power with checked products
        cmp eval_right, 0                ; Integer powers require a nonnegative exponent
        jl apply_operator_failed        ; Reject a negative exponent
        mov ax, 1                        ; The exponent-zero identity is one
        mov eval_accumulator, ax         ; Initialize the repeated-multiplication result

    power_multiply_loop: ; Multiply the accumulator once for each remaining exponent unit
        cmp eval_right, 0                ; Check whether all exponent multiplications are done
        je power_result_ready            ; Return one for exponent zero, or the accumulated power
        mov ax, eval_accumulator         ; AX = current power accumulator
        mov dx, eval_left                ; DX = base
        imul dx                          ; DX:AX = accumulator multiplied by base
        call check_product_range         ; Verify each intermediate product fits signed 16-bit
        jc apply_operator_failed        ; Reject an overflowing intermediate power
        mov eval_accumulator, ax         ; Save the valid next power
        dec eval_right                   ; Count down one multiplication
        jmp power_multiply_loop          ; Continue until exponent reaches zero

    power_result_ready: ; Transfer the completed power to the common result path
        mov ax, eval_accumulator         ; AX = final integer power

    apply_operation_result: ; Push a valid arithmetic result for subsequent tokens
        call push_eval_value             ; Push the operator result for the next token
        jc apply_operator_failed         ; Reject a full evaluation stack
        clc                              ; Return a successfully applied operator
        ret                              ; Return after pushing the result

    apply_operator_failed: ; Return carry for any operand, arithmetic, or stack failure
        stc                              ; Return any operator failure to the evaluator
        ret                              ; Return without presenting a final result
apply_postfix_operator endp ; End binary operator dispatch and arithmetic


; -----------------------------------------
; Check whether a signed double-word product fits in one signed word
; Input:  DX:AX = signed 32-bit product
; Output: CF=0 when AX is the valid signed 16-bit product; CF=1 on overflow
; Preserves: AX, BX, CX, DX, SI, DI, BP
; Clobbers: flags
; Conventions: High word must be the sign extension of bit 15 in AX
; Error behavior: Reports values outside -32768 through 32767 with CF=1
; -----------------------------------------
check_product_range proc ; Begin signed double-word product range validation
    cmp dx, 0                        ; Check the positive or zero high word case
    je product_high_word_zero        ; A zero high word requires a nonnegative low word
    cmp dx, 0FFFFh                   ; Check for sign extension of a negative low word
    jne product_out_of_range         ; Any other high word cannot fit in signed 16 bits
    test ax, 8000h                   ; A negative low word must have its sign bit set
    jz product_out_of_range          ; Reject a negative high word with nonnegative low word
    clc                              ; The negative product fits in signed 16-bit range
    ret                              ; Return success while preserving DX:AX

    product_high_word_zero: ; Validate a nonnegative product's low-word sign bit
        test ax, 8000h                   ; A nonnegative product must have sign bit clear
        jnz product_out_of_range         ; Reject a positive high word with negative low word
        clc                              ; The nonnegative product fits in signed 16-bit range
        ret                              ; Return success while preserving DX:AX

    product_out_of_range: ; Report that the product does not fit a signed word
        stc                              ; Report a signed 16-bit multiplication overflow
        ret                              ; Return failure while preserving DX:AX
check_product_range endp ; End signed double-word product range validation
