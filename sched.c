/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <hardware.h>

char initial_stack[KERNEL_STACK_SIZE] __attribute__ ((aligned(4096))); // Space for the initial system stack
struct task_struct * init_task;  
struct task_struct * idle_task;
struct list_head readyqueue;


void cpu_idle(void)
{
	__sti();
	while(1)
	{
	;
	}
}

void init_idle (void)
{
	int Dir = alloc_frame(); //page directory for the process
	page_table_entry *DirAddress_idle = (page_table_entry *) (Dir << 12);
	clear_page_table(DirAddress_idle);

	int frame_PTSys = init_task->dir_pages_baseAddr[0].bits.pbase_addr; //find system PT frame
	page_table_entry *PT0 = (page_table_entry *) (frame_PTSys << 12);
	
	set_ss_pag(PT0, Dir, Dir, 0); //Map the new directory in the system PT

	set_ss_pag(DirAddress_idle, 0, frame_PTSys, 0); //Map the system PT in the page directory of idle process
	
	int PCBframe = alloc_frame(); //allocate new struct task_struct
	struct task_struct *idlePCB = (struct task_struct *) (PCBframe << 12);
	set_ss_pag(PT0, PCBframe, PCBframe, 0); //mapping PCB to system PT
	idlePCB->PID = 0; //assigning PID 0 to the idle process
	idlePCB->dir_pages_baseAddr = DirAddress_idle; //initialize directory with DirAddress_idle

	//PREPARE STACK
	/*
	-----------
	| %EBP    | <-- stack[KERNEL_STACK_SIZE - 2]
	-----------
	|@HANDLER | <-- stack[KERNEL_STACK_SIZE - 1]
	-----------
	*/

	union task_union *idle_union = (union task_union *)idlePCB; 
	idle_union->stack[KERNEL_STACK_SIZE - 1] = (unsigned long) &cpu_idle; //store in stack address of function idle
	idle_union->stack[KERNEL_STACK_SIZE - 2] = 0;	//store stack initial value of %ebp (0)
	idlePCB->kernel_esp = &(idle_union->stack[KERNEL_STACK_SIZE - 2]); //the position of the stack with the initial EBP

	idle_task = idlePCB;

}

void init_task1(void)
{
	int Dir = alloc_frame(); //allocate a frame for the page directory 
	/* Dir has page frame number */
	// DirAddress has the address of the frame Dir (Dir * framesize = Dir << 12)
	page_table_entry *DirAddress = (page_table_entry *) (Dir << 12); 
	clear_page_table(DirAddress); //clear the contents of the frame

	int PTS = alloc_frame(); //allocate a frame fot the system PT to store system mapping
	page_table_entry *PT0 = (page_table_entry *) (PTS << 12); 
	clear_page_table(PT0); //clear the contetns of the frame
	set_kernel_pages(PT0); //fill up the system PT with its logical to physical addresses

	int PTU = alloc_frame(); //allocate PT to store user mapping
	page_table_entry *PT1 = (page_table_entry *) (PTU << 12);
	clear_page_table(PT1);
	set_user_pages(PT1);

	set_ss_pag(PT0, Dir, Dir, 0); //adds an entry to system PT with the mapping of page directory
	set_ss_pag(PT0, PTS, PTS, 0); //adds an entry to system PT with the mapping of its own PT
	set_ss_pag(PT0, PTU, PTU, 0); //adds an entry to system PT with the mapping of user PT

	set_ss_pag(DirAddress, 0, PTS, 0); //first page directory entry has system PT
	set_ss_pag(DirAddress, 1, PTU, 1); // second page directory entru has user PT

	
	int PCBframe = alloc_frame();
	/* más adelante podemos hacer casting (union task_union *) para tratarlo como task_union.
	Como ambos comienzan en la misma dirección de memoria, ntambién podríamos haber decarado 
	*newPCB como union task_union. Hemos usado task_struct porque lo dice así la documentación*/
	struct task_struct *newPCB = (struct task_struct *) (PCBframe << 12);
	set_ss_pag(PT0, PCBframe, PCBframe, 0); //mapping PCB to system PT
	newPCB->PID = 1; //assigning PID 1 to the first process

	tss.esp0 = KERNEL_ESP((union task_union *) (newPCB));
	newPCB->dir_pages_baseAddr = DirAddress;//initialize the directory 
	set_cr3(DirAddress); //make it the current page directory
	init_task = newPCB;
}

void init_sched()
{
	INIT_LIST_HEAD(&readyqueue);
}

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
       return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
       return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}

/* get address of task_struct given its list_head*/
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
	return list_entry(l, struct task_struct, list);
}
