; -----------------------------------------
; Push operator onto custom stack
; Input:  AL = operator
; Output: CF = 0 on success, 1 if full
; Preserves: BX
; Clobbers: Flags
; Conventions: stack_top counts occupied byte entries; AL is stored at the next free index
; Error behavior: Returns CF=1 without modifying the stack when full
; -----------------------------------------
push_operator proc ; Begin the bounded operator-stack push procedure
    push bx                   ; Save BX before using it

    cmp stack_top, 100        ; Check maximum stack capacity
    jae push_full             ; No free position if index >= 100

    mov bx, stack_top         ; Get index of next free position
    mov [operator_stack + bx], al ; Store operator from AL
    inc stack_top             ; Advance next-free index

    clc                       ; Clear carry flag: success
        jmp push_done        ; Restore BX and return

    push_full: ; Report that the operator stack has no free entry
        stc                   ; Set carry flag: stack overflow

    push_done: ; Restore BX and return the carry status
        pop bx                ; Restore BX (does not modify flags)
        ret                   ; Return carry status to the caller

push_operator endp ; End the bounded operator-stack push procedure


; -----------------------------------------
; Pop operator from custom stack
; Output: AL = popped operator on success
;         CF = 0 on success, 1 if empty
; Preserves: BX
; Clobbers: AL, flags
; Conventions: stack_top counts occupied byte entries; the top entry is removed first
; Error behavior: Returns CF=1 without modifying the stack when empty
; -----------------------------------------
pop_operator proc ; Begin the bounded operator-stack pop procedure
    push bx                   ; Preserve BX

    cmp stack_top, 0          ; Check whether stack is empty
        je pop_empty         ; Handle an empty operator stack

    dec stack_top             ; Move index back to last stored item
    mov bx, stack_top         ; BX = index of top operator
    mov al, [operator_stack + bx] ; Retrieve operator into AL

    clc                       ; Clear carry flag: success
        jmp pop_done         ; Restore BX and return

    pop_empty: ; Report that no operator is available to remove
        stc                   ; Set carry flag: stack underflow

    pop_done: ; Restore BX and return the carry status
        pop bx                ; Restore BX without changing flags
        ret                   ; Return carry status to the caller

pop_operator endp ; End the bounded operator-stack pop procedure


; -----------------------------------------
; Get arithmetic operator precedence
; Input:  AL = operator
; Output: AH = precedence
; Preserves: AL
; Clobbers: AH, flags
;
; 0 = '(' or unknown
; 1 = '+' or '-'
; 2 = '*' or '/'
; 3 = '^'
; Error behavior: Unknown operators retain the documented default precedence zero
; -----------------------------------------
get_precedence proc ; Begin operator precedence lookup
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

    one_precedence: ; Return precedence one for addition or subtraction
        mov ah, 1             ; Addition/subtraction precedence
        jmp precedence_done  ; Return the selected precedence

    two_precedence: ; Return precedence two for multiplication or division
        mov ah, 2             ; Multiplication/division precedence
        jmp precedence_done  ; Return the selected precedence

    three_precedence: ; Return precedence three for exponentiation
        mov ah, 3             ; Exponentiation precedence
        jmp precedence_done  ; Return the selected precedence

    precedence_done: ; Return the selected precedence in AH
        ret                   ; Return precedence in AH

get_precedence endp ; End operator precedence lookup
