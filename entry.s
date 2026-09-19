# 0 "entry.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "entry.S"




# 1 "include/asm.h" 1
# 6 "entry.S" 2
# 1 "include/segment.h" 1
# 7 "entry.S" 2
# 60 "entry.S"
      popl %ecx;
      popl %ebx;
      popl %esi;
      popl %edi;
      popl %ebp;
      popl %eax;
      popl %edx;
      popl %ds;
      popl %es;
      popl %fs;
      popl %gs;

.globl pf_handler; .type pf_handler, @function; .align 0; pf_handler:
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es

 pushl %esp;
 call pf_routine;
 addl $4, %esp;
 popl %edx;
 addl $4, %esp;
 iret;
