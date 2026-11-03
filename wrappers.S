#include <asm.h>

//TESTEAR ERRORES
ENTRY(test_enosys)
      pushl %ebp
      movl %esp, %ebp
      movl $0, %eax
      int $0x93
      popl %ebp
      ret

ENTRY(test_sys_write_bad_buff)
      pushl %ebp
      movl %esp, %ebp
      pushl %ebx          
      movl $0, %ebx       
      movl $4, %eax       
      int $0x93
      popl %ebx
      popl %ebp
      ret
//TESTEAR ERRORES

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

ENTRY(getpid)
    pushl %ebp
    movl %esp, %ebp
    
    pushl %ebx
    movl $20, %eax
    int $0x93

    popl %ebx
    movl %ebp, %esp
    popl %ebp
    ret  

ENTRY(fork)
    pushl %ebp
    movl %esp, %ebp
    
    pushl %ebx
    movl $2, %eax
    int $0x93

    test %eax, %eax
    jns np

    negl %eax
    movl %eax, errno
    movl $-1, %eax

np: 
    movl %ebp, %esp
    popl %ebp

    ret  
