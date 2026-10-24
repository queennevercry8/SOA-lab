#include <asm.h>

ENTRY(write)
    pushl %ebp
    movl %esp, %ebp
    
    pushl %ebx
    
    leal 8(%ebp), %ebx

    //movl 8(%ebp), %edx
    //movl 12(%ebp), %ecx
    //movl 16(%ebp), %ebx

    movl $4, %eax //identifier of the syscall in the register eax (look at sys_call_table)
    int $0x93

    popl %ebx
    
    test %eax, %eax
    jns ok

    negl %eax
    movl %eax, errno
    movl $-1, %eax

ok: movl %ebp, %esp
    popl %ebp

    ret  

ENTRY(gettime)
    pushl %ebp
    movl %esp, %ebp

    pushl %ebx
    movl $10, %eax
    int $0x93

    popl %ebx
    
    test %eax, %eax
    jns fine

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fine: 
    movl %ebp, %esp
    popl %ebp

    ret  

