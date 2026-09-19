#include <asm.h>
ENTRY(addAsm)
/* your assembly instructions separated by newlines (\n) */
	push %ebp
	mov %esp, %ebp
	mov 0x8(%ebp), %edx
	mov 0xc(%ebp), %eax
	add %edx, %eax
	pop %ebp
	ret 
