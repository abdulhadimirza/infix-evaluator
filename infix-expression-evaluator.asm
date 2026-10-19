.model small                  ; One code segment and one data segment
.stack 100h                   ; Reserve 256 bytes for CPU stack

.data
include 'app_data.inc'

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

include 'console_input.inc'
include 'infix_to_postfix.inc'
include 'operator_utils.inc'
include 'console_output.inc'

end main                      ; Program entry point
