; 上下文切换的汇编实现
; void co_swap(CoContext* from, CoContext* to)
global co_swap
section .text
co_swap:
    ; 保存调用者的非易失性寄存器
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ; 如果from不为空，保存当前上下文
    cmp rdi, 0
    je .load_context
    mov [rdi + 0x00], rbx  ; 保存rbx
    mov [rdi + 0x08], rbp  ; 保存rbp
    mov [rdi + 0x10], r12  ; 保存r12
    mov [rdi + 0x18], r13  ; 保存r13
    mov [rdi + 0x20], r14  ; 保存r14
    mov [rdi + 0x28], r15  ; 保存r15
    
    ; 保存当前栈指针rsp
    mov rax, rsp
    mov [rdi + 0x30], rax  ; 保存rsp
    
    ; 保存返回地址作为rip
    mov rax, [rsp]         ; 获取栈顶的返回地址
    mov [rdi + 0x38], rax  ; 保存rip
    
    ; 调整栈指针，移除返回地址
    add rsp, 8

.load_context:
    ; 加载目标上下文
    mov rbx, [rsi + 0x00]  ; 恢复rbx
    mov rbp, [rsi + 0x08]  ; 恢复rbp
    mov r12, [rsi + 0x10]  ; 恢复r12
    mov r13, [rsi + 0x18]  ; 恢复r13
    mov r14, [rsi + 0x20]  ; 恢复r14
    mov r15, [rsi + 0x28]  ; 恢复r15
    
    ; 恢复栈指针rsp
    mov rsp, [rsi + 0x30]  ; 恢复rsp
    
    ; 跳转到目标rip
    jmp qword [rsi + 0x38] ; 跳转到目标指令指针
    