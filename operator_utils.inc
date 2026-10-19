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
