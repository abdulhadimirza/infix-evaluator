.model small                  ; One code segment and one data segment
.stack 100h                   ; Reserve 256 bytes for CPU stack

.data ; Begin the program's data segment
include 'app_data.inc' ; Declare prompts, input buffers, and evaluator state

.code ; Begin the program's code segment

; -----------------------------------------
; Main procedure
; Input: None
; Output: Runs the read, convert, evaluate, display, and continue loop
; Preserves: None
; Clobbers: AX, BX, CX, DX, SI, DI, flags
; Conventions: DS is initialized before any data access; carry reports evaluation failure
; Error behavior: Invalid input or evaluation returns to the expression prompt
; -----------------------------------------
main proc ; Begin program initialization and expression workflow
    mov ax, @data             ; Load data segment address into AX
    mov ds, ax                ; Initialize DS to access .data variables

    mov ah, 09h               ; DOS function 09h: print string
    lea dx, app_title         ; DX = address of title
    int 21h                   ; Call DOS interrupt

    restart_program: ; Reset per-expression state and read another expression
        mov stack_top, 0      ; Reset operator stack
        mov postfix_len, 0    ; Reset postfix length
        mov error_flag, 0     ; Clear previous error
        mov eval_top, 0       ; Reset evaluation stack before processing another expression
        mov result, 0         ; Clear the previous expression result

        call read_expression   ; Read a new expression from the keyboard
        call convert_to_postfix ; Validate it and build postfix output

        cmp error_flag, 1     ; Check whether conversion failed
        je display_error      ; Jump if error_flag == 1

        call print_postfix    ; Display the converted expression
        call evaluate_postfix ; Evaluate the postfix tokens into the result variable
        jc display_evaluation_error ; Retry when evaluation reports invalid arithmetic or stack state
        call print_signed_result ; Display the final result as a signed decimal number
        call ask_to_continue  ; Ask whether to process another expression

        cmp al, 'Y'           ; Check whether the user chose to continue
        je restart_program    ; Start another evaluation after Y
        jmp exit_program      ; N exits the program

    display_error: ; Print the conversion error before restarting input
        mov ah, 09h           ; DOS string output function
        lea dx, error_prompt  ; Load error message address
        int 21h                 ; Display the invalid-expression message

        jmp restart_program   ; Retry with a new expression

    display_evaluation_error: ; Print arithmetic or postfix evaluation failure before retry
        mov ah, 09h           ; DOS string output function
        lea dx, evaluation_error_prompt ; Load the evaluation failure message
        int 21h               ; Display the evaluation error
        jmp restart_program   ; Retry with a new expression

main endp ; End program initialization and expression workflow

include 'console_input.inc' ; Include keyboard input procedures
include 'infix_to_postfix.inc' ; Include the infix-to-postfix converter
include 'operator_utils.inc' ; Include the converter's operator-stack utilities
include 'console_output.inc' ; Include labeled console output procedures
include 'postfix_evaluation.inc' ; Include postfix parsing and arithmetic procedures

end main                      ; Program entry point
