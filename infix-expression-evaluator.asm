.model small                  ; One code segment and one data segment
.stack 100h                   ; Reserve 256 bytes for CPU stack

.data
    ; DOS function 09h displays '$'-terminated strings
    app_title db 'Infix Expression Evaluator', 13, 10, '$' ; Title displayed at startup
    input_prompt db 13, 10, 'Enter an expression: $' ; Prompt shown before each input

    ; DOS 0Ah buffer: maximum length, actual length, input characters
    expression db 100, 0, 100 dup(?) ; DOS input buffer: maximum, actual length, then characters
    expr_len db 0              ; Store actual input length separately

    operator_stack db 100 dup(?) ; Operator stack (separate from CPU stack)
    stack_top dw 0             ; Index of next free position (0 = empty)

    postfix db 200 dup(?)     ; Output buffer with space for token separators
    postfix_len db 0          ; Number of output characters

    error_flag db 0           ; 1 indicates invalid expression
    expect_operand db 1       ; 1 = expect number or '(', 0 = expect operator or ')'
    error_prompt db 13, 10, 'Invalid expression. Try again.', 13, 10, '$'
    continue_prompt db 13, 10, 13, 10, 'Evaluate another expression? (Y/N): $' ; Ask whether to restart
    continue_error_prompt db 13, 10, 'Please enter Y or N.', 13, 10, '$' ; Explain invalid choice

    postfix_prompt db 13, 10, 'Postfix: $' ; Label before converted output
    
    evaluation_stack dw 100 dup(?)  ; Holds up to 100 signed 16-bit numbers
    eval_top dw 0                   ; Next free evaluation stack index
    result dw 0                     ; Final evaluated result
    
    exit_prompt db 13, 10, 'Press any key to exit...', 13, 10, '$' ; Pause before termination

.code

; -----------------------------------------
; Main procedure
; -----------------------------------------
main proc
    mov ax, @data             ; Load data segment address into AX
    mov ds, ax                ; Initialize DS to access .data variables

    mov ah, 09h               ; DOS function 09h: print string
    lea dx, app_title         ; DX = address of title
    int 21h                   ; Call DOS interrupt

    restart_program:
        mov stack_top, 0      ; Reset operator stack
        mov postfix_len, 0    ; Reset postfix length
        mov error_flag, 0     ; Clear previous error

        call read_expression   ; Read a new expression from the keyboard
        call convert_to_postfix ; Validate it and build postfix output

        cmp error_flag, 1     ; Check whether conversion failed
        je display_error      ; Jump if error_flag == 1

        call print_postfix    ; Display the converted expression
        call ask_to_continue  ; Ask whether to process another expression

        cmp al, 'Y'           ; Check whether the user chose to continue
        je restart_program    ; Start another evaluation after Y
        jmp exit_program      ; N exits the program

    display_error:
        mov ah, 09h           ; DOS string output function
        lea dx, error_prompt  ; Load error message address
        int 21h                 ; Display the invalid-expression message

        jmp restart_program   ; Retry with a new expression

main endp


; -----------------------------------------
; Ask whether to evaluate another expression
; Output: AL = 'Y' or 'N'
; -----------------------------------------
ask_to_continue proc
    ask_again:
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

    continue_choice_done:
        ret                      ; Return the accepted choice in AL
ask_to_continue endp


; -----------------------------------------
; Read expression from keyboard
; Input:  None
; Output: expression, expr_len
; -----------------------------------------
read_expression proc
    mov ah, 09h               ; Display input prompt
    lea dx, input_prompt      ; DX = address of the input prompt
    int 21h                   ; Display the prompt

    mov ah, 0Ah               ; DOS buffered keyboard input
    lea dx, expression        ; DX = address of input buffer
    int 21h                   ; Read the expression into the DOS buffer

    mov al, [expression + 1]  ; Second buffer byte holds input length
    mov [expr_len], al        ; Save length for conversion procedure

    ret                      ; Return after saving the input length
read_expression endp


; -----------------------------------------
; Convert infix expression to postfix
; Using the shunting-yard algorithm
;
; SI = current input character address
; DI = next free postfix output address
; BX = address immediately after input
; AL = current character/operator
; AH = operator precedence
; -----------------------------------------
convert_to_postfix proc
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
    convert_loop:
        cmp si, bx            ; Have we reached the end of input?
        jae flush_stack       ; JAE = unsigned above or equal

        mov al, [si]          ; Load current character

        cmp al, '0'           ; Check whether character is a digit
        jb check_symbols      ; Below '0' means not a digit
        cmp al, '9'           ; Compare with the highest decimal digit
        jbe output_operand    ; Between '0' and '9' inclusive

    check_symbols:
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
    output_operand:
        cmp expect_operand, 1 ; Reject adjacent numbers or a number after ')'
        jne conversion_error ; Reject a number after another operand

    copy_number:
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

    number_done:
        mov byte ptr [di], ' ' ; Separate postfix tokens
        inc di                ; Advance to next output position
        mov expect_operand, 0 ; A number was read; next expect operator or ')'

        jmp convert_loop      ; SI already points to next non-digit


    ; -----------------------------------------
    ; Handle arithmetic operator
    ; Pop higher-precedence operators first
    ; Pop equal precedence if left-associative
    ; -----------------------------------------
    handle_operator:
        cmp expect_operand, 1 ; Reject leading or repeated binary operators
        jne operator_is_valid ; Continue if an operand precedes this operator
        jmp conversion_error ; Record invalid operator placement

    operator_is_valid:
        mov expect_operand, 1 ; An operator must be followed by an operand
        call get_precedence   ; AL = operator, AH = precedence
        push ax               ; Save incoming operator and precedence

    operator_loop:
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

    pop_top_operator:
        call pop_operator     ; Remove top operator, return it in AL
        jc operator_conversion_error ; CF = 1 means underflow

        mov [di], al          ; Append popped operator to postfix
        inc di                ; Advance to the next output position
        mov byte ptr [di], ' ' ; Add token separator
        inc di                ; Advance beyond emitted operator

        jmp operator_loop     ; Check remaining stacked operators

    push_current_operator:
        pop ax                ; Restore original operator into AL
        call push_operator    ; Push operator onto custom stack
        jc conversion_error   ; CF = 1 means stack overflow

        jmp next_character   ; Continue scanning after pushing operator


    ; -----------------------------------------
    ; Handle opening parenthesis '('
    ; -----------------------------------------
    handle_left_paren:
        cmp expect_operand, 1 ; Reject implicit multiplication, such as 2(3)
        jne conversion_error ; Reject '(' when an operand is not expected
        call push_operator    ; Push '(' as precedence boundary
        jc conversion_error

        jmp next_character   ; Continue scanning inside the group


    ; -----------------------------------------
    ; Handle closing parenthesis ')'
    ; Pop until matching '(' is found
    ; -----------------------------------------
    handle_right_paren:
        cmp expect_operand, 1 ; Reject empty parentheses or a missing operand
        je conversion_error  ; Reject ')' when no operand precedes it

    right_paren_loop:
        call pop_operator     ; Pop next operator into AL
        jc conversion_error   ; Empty stack means unmatched ')'

        cmp al, '('           ; Found matching opening parenthesis?
        je right_paren_matched ; Matching '(' ends the pop loop

        mov [di], al          ; Append operator to postfix
        inc di                ; Advance to the next output position
        mov byte ptr [di], ' ' ; Append separator
        inc di                ; Advance beyond emitted operator

        jmp right_paren_loop  ; Continue until '(' is found

    right_paren_matched:
        mov expect_operand, 0 ; A closing parenthesis completes an operand
        jmp next_character   ; Continue after closing the group


    ; -----------------------------------------
    ; Advance input pointer
    ; -----------------------------------------
    next_character:
        inc si                ; Move to next input character
        jmp convert_loop     ; Process the next input character


    ; -----------------------------------------
    ; Flush remaining stacked operators
    ; -----------------------------------------
    flush_stack:
        cmp expect_operand, 1 ; Reject an expression ending with an operator
        je conversion_error  ; Reject expression ending with an operator
        cmp stack_top, 0      ; Check whether stack is empty
        je conversion_done    ; Conversion finished when empty

        call pop_operator     ; Remove remaining operator
        jc conversion_error

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
    operator_conversion_error:
        pop dx                ; Discard saved incoming operator
        jmp conversion_error ; Route through the common error handler

    conversion_error:
        mov error_flag, 1     ; Inform main procedure of failure
        jmp conversion_done  ; Return after setting the error flag

    conversion_done:
        ret                   ; DI points immediately after output

convert_to_postfix endp


; -----------------------------------------
; Push operator onto custom stack
; Input:  AL = operator
; Output: CF = 0 on success, 1 if full
; Preserves: BX
; -----------------------------------------
push_operator proc
    push bx                   ; Save BX before using it

    cmp stack_top, 100        ; Check maximum stack capacity
    jae push_full             ; No free position if index >= 100

    mov bx, stack_top         ; Get index of next free position
    mov [operator_stack + bx], al ; Store operator from AL
    inc stack_top             ; Advance next-free index

    clc                       ; Clear carry flag: success
        jmp push_done        ; Restore BX and return

    push_full:
        stc                   ; Set carry flag: stack overflow

    push_done:
        pop bx                ; Restore BX (does not modify flags)
        ret                   ; Return carry status to the caller

push_operator endp


; -----------------------------------------
; Pop operator from custom stack
; Output: AL = popped operator on success
;         CF = 0 on success, 1 if empty
; Preserves: BX
; -----------------------------------------
pop_operator proc
    push bx                   ; Preserve BX

    cmp stack_top, 0          ; Check whether stack is empty
        je pop_empty         ; Handle an empty operator stack

    dec stack_top             ; Move index back to last stored item
    mov bx, stack_top         ; BX = index of top operator
    mov al, [operator_stack + bx] ; Retrieve operator into AL

    clc                       ; Clear carry flag: success
        jmp pop_done         ; Restore BX and return

    pop_empty:
        stc                   ; Set carry flag: stack underflow

    pop_done:
        pop bx                ; Restore BX without changing flags
        ret                   ; Return carry status to the caller

pop_operator endp


; -----------------------------------------
; Get arithmetic operator precedence
; Input:  AL = operator
; Output: AH = precedence
;
; 0 = '(' or unknown
; 1 = '+' or '-'
; 2 = '*' or '/'
; 3 = '^'
; -----------------------------------------
get_precedence proc
    mov ah, 0                 ; Default precedence = 0

        cmp al, '+'           ; Check addition
        je one_precedence    ; Return addition precedence
        cmp al, '-'           ; Check subtraction
        je one_precedence    ; Return subtraction precedence

        cmp al, '*'           ; Check multiplication
        je two_precedence    ; Return multiplication precedence
        cmp al, '/'           ; Check division
        je two_precedence    ; Return division precedence

        cmp al, '^'           ; Check exponentiation
        je three_precedence  ; Return exponentiation precedence

    jmp precedence_done       ; Unknown operator keeps precedence 0

    one_precedence:
        mov ah, 1             ; Addition/subtraction precedence
        jmp precedence_done  ; Return the selected precedence

    two_precedence:
        mov ah, 2             ; Multiplication/division precedence
        jmp precedence_done  ; Return the selected precedence

    three_precedence:
        mov ah, 3             ; Exponentiation precedence
        jmp precedence_done  ; Return the selected precedence

    precedence_done:
        ret                   ; Return precedence in AH

get_precedence endp


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

end main                      ; Program entry point
