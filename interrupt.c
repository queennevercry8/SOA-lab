/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

typedef struct stack{
	//contexto SW
	int edx;
	int ecx;
	int ebx;
	int esi;
	int edi;
	int ebp;
	int eax;

	int ds;
	int es;
	int fs;
	int gs;


	//contexto HW del PF
	int error_code;
	int eip;
	int cs;
	int eflags;
	int esp;
	int ss;
} page_fault_stack_t;

char* register_name[] = {"EDX","ECX","EBX","ESI","EDI","EBP","EAX","DS","ES","FS","GS","ERROR_CODE","EIP","CS","EFLAGS","ESP","SS"};

void itoa(int a, char *word)
{
  char hex_digits[] = "0123456789ABCDEF";
  int i, reverse;
  char c;
  
  if (a==0) { word[0]='0'; word[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    word[i]=hex_digits[(a%16)];
    a=a/16;
    i++;
  }
  
  for (reverse=0; reverse<i/2; reverse++)
  {
    c=word[reverse];
    word[reverse]=word[i-reverse-1];
    word[i-reverse-1]=c;
  }
  word[i]=0;
}


void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(14, pf_handler, 0);
  set_idt_reg(&idtR);
}

//------------------------------------------------------------------------
void pf_routine(page_fault_stack_t *stack){
	int cr2 = read_cr2();
	char hex_direction[10];
	itoa(cr2, hex_direction);

	printk(":< Offending address: 0x");
	printk(hex_direction);
	printk("\n");	

	printk("Process generates a PAGE FAULT exception at EIP: 0x");
	itoa(stack->eip, hex_direction);
	printk(hex_direction);
	printk("\n");

	printk("CPU registers value: \n");
	int *regs = (int *) stack;
	for(int i = 0; i < 17; ++i)
	{
		printk(register_name[i]);
		printk(": 0x");
		itoa(regs[i], hex_direction);
		printk(hex_direction);
		printk("\n");
	}

	while(1);
}



