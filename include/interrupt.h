/*
 * interrupt.h - Definició de les diferents rutines de tractament d'exepcions
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <types.h>

#define IDT_ENTRIES 256

extern Gate idt[IDT_ENTRIES];
extern Register idtR;

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL);
void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL);

void setIdt();

typedef struct {
    unsigned int edx;
    unsigned int ecx;
    unsigned int ebx;
    unsigned int esi;
    unsigned int edi;
    unsigned int ebp;
    unsigned int eax;
    unsigned int ds;
    unsigned int es;
    unsigned int fs;
    unsigned int gs;
    unsigned int error_code;
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
    unsigned int esp;
    unsigned int ss;
} page_fault_stack_t;

extern char* itoa_hex(int a, char *b);

extern void pf_handler(void);
void pf_routine(page_fault_stack_t *stack);

extern void keyboard_handler(void);

void keyboard_routine();

extern void clock_handler(void);
void clock_routine();

#endif  /* __INTERRUPT_H__ */
