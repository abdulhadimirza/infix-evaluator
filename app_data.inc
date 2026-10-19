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
    error_prompt db 13, 10, 'Invalid expression. Try again.', 13, 10, '$' ; Explain invalid infix syntax
    evaluation_error_prompt db 13, 10, 'Unable to evaluate expression. Try again.', 13, 10, '$' ; Explain arithmetic or postfix evaluation failure
    continue_prompt db 13, 10, 13, 10, 'Evaluate another expression? (Y/N): $' ; Ask whether to restart
    continue_error_prompt db 13, 10, 'Please enter Y or N.', 13, 10, '$' ; Explain invalid choice

    postfix_prompt db 13, 10, 'Postfix: $' ; Label before converted output
    result_prompt db 13, 10, 'Result: $' ; Start the signed decimal result on its own line
    
    evaluation_stack dw 100 dup(?)  ; Holds up to 100 signed 16-bit numbers
    eval_top dw 0                   ; Next free evaluation stack index
    result dw 0                     ; Final evaluated result
    eval_pos dw 0                   ; Current character offset while scanning postfix output
    eval_left dw 0                  ; Left operand for the current binary operation
    eval_right dw 0                 ; Right operand or remaining exponent for the current operation
    eval_accumulator dw 0            ; Running result for integer exponentiation
    eval_digit db 0                 ; Current decimal digit while parsing a postfix operand
    current_operator db 0           ; Operator currently being applied to the evaluation stack
    
    exit_prompt db 13, 10, 'Press any key to exit...', 13, 10, '$' ; Pause before termination
