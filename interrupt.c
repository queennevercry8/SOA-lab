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

char* register_name[] = {"EDX","ECX","EBX","ESI","EDI","EBP","EAX","DS","ES","FS","GS","ERROR_CODE","EIP","CS","EFLAGS","ESP","SS"};

static const unsigned char char_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6',   /* 0x00 - 0x07 */
    '7', '8', '9', '0', '-', '=', '\b','\t',  /* 0x08 - 0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',   /* 0x10 - 0x17 */
    'o', 'p', '[', ']', '\n', 0,  'a', 's',   /* 0x18 - 0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',   /* 0x20 - 0x27 */
    '\'','`', 0,   '\\','z', 'x', 'c', 'v',   /* 0x28 - 0x2F */
    'b', 'n', 'm', ',', '.', '/', 0,   '*',   /* 0x30 - 0x37 */
    0,   ' ', 0,   0,   0,   0,   0,   0,     /* 0x38 - 0x3F */
    0,   0,   0,   0,   0,   0,   0,   '7',   /* 0x40 - 0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',   /* 0x48 - 0x4F */
    '2', '3', '0', '.', 0,   0,   0,   0,     /* 0x50 - 0x57 */
    0,   0,   0,   0,   0,   0,   0,   0,     /* 0x58 - 0x5F */
    0,   0,   0,   0,   0,   0,   0,   0,     /* 0x60 - 0x67 */
    0,   0,   0,   0,   0,   0,   0,   0,     /* 0x68 - 0x6F */
    0,   0,   0,   0,   0,   0,   0,   0,     /* 0x70 - 0x77 */
    0,   0,   0,   0,   0,   0,   0,   0      /* 0x78 - 0x7F */
};

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
  setInterruptHandler(33, keyboard_handler, 0);
  setInterruptHandler(32, clock_handler, 0);

  setTrapHandler(0x93, system_call_handler, 3);

  set_idt_reg(&idtR);
}

//------------------------------------------------------------------------
void pf_routine(page_fault_stack_t *stack){
	int cr2 = read_cr2();
	char hex_direction[10];

  printk("\n");
	printk("Process generates a PAGE FAULT exception at EIP: ");
	printk(itoa_hex(stack->eip, hex_direction));
	printk("\n");

  
	printk("Offending address: ");
	printk(itoa_hex(cr2, hex_direction));
	printk("\n");	

	printk("CPU registers value: \n");
	int *regs = (int *) stack;
	for(int i = 0; i < 17; ++i)
	{
		printk(register_name[i]);
		printk(": ");
		printk(itoa_hex(regs[i], hex_direction));
		printk("\n");
	}

	while(1);
}

void keyboard_routine() {
    unsigned char scan_code;
    unsigned char key;
    unsigned char is_make;

    scan_code = inb(0x60); //port corresponding to the keyboard data register is 0x60

    /* Bit 7 indicates make (0) or break (1) */
    is_make = (scan_code & 0x80) == 0;

    if (is_make) {
        /* Mask the high bit to get the scan code index */
        key = char_map[scan_code & 0x7F];

        /* If the key is not a printable ASCII character, show 'C' */
        if (key == 0 || key < 32 || key > 126) key = 'C';
        printc_xy(0, 0, key);
    }
}

void clock_routine() {
    zeos_show_clock();
    zeos_ticks++;
}

