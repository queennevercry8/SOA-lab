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
struct list_head zombiequeue;

/* Ticks remaining for the process currently running on the CPU */
static int ticks_left;

void cpu_idle(void)
{
	__sti();
	printk("First time entering cpu_idle!\n");
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


	/*ADDED*/
	idlePCB->state = ST_READY; //idle is not running yet
	idlePCB->quantum = DEFAULT_QUANTUM;
	INIT_LIST_HEAD(&idlePCB->list); //idle is never inserted into readyqueue

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

	/*ADDED*/
	newPCB->state = ST_RUN; //init is the process currently running
	newPCB->quantum = DEFAULT_QUANTUM;
	INIT_LIST_HEAD(&newPCB->list);

	tss.esp0 = KERNEL_ESP((union task_union *) (newPCB));
	newPCB->dir_pages_baseAddr = DirAddress;//initialize the directory 
	set_cr3(DirAddress); //make it the current page directory
	init_task = newPCB;
}

void init_sched()
{
	global_pid = 2;
	INIT_LIST_HEAD(&readyqueue);
	INIT_LIST_HEAD(&zombiequeue);
	ticks_left = DEFAULT_QUANTUM;
}

/* Libera los PCB de procesos ya muertos. Se llama desde otro contexto,
   así que el frame que liberamos no es la pila actual. */
void free_zombies(void)
{
    page_table_entry *sysPT = get_PT(current());

    while (!list_empty(&zombiequeue)) {
        struct list_head *l = list_first(&zombiequeue);
        struct task_struct *z = list_head_to_task_struct(l);
        int frame = ((unsigned long)z) >> 12;

        list_del(l);
        del_ss_pag(sysPT, frame);   /* quitar el mapeo identidad del PCB */
        free_frame(frame);          /* liberar el frame del PCB/pila */
    }
}

/* get_quantum - Returns the quantum of task 't' */
int get_quantum(struct task_struct *t)
{
	return t->quantum;
}

/* set_quantum - Changes the quantum of task 't' */
void set_quantum(struct task_struct *t, int new_quantum)
{
	if (new_quantum > 0)
		t->quantum = new_quantum;
}

/* update_sched_data_rr - Updates the ticks left of the current process */
void update_sched_data_rr(void)
{
	if (current() != idle_task && ticks_left > 0)
		--ticks_left;
}

/* update_process_state_rr - Moves 't' to the queue for its new state.
   dst_queue == NULL means that the process is going to run. */
void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
	if (t->state != ST_RUN)
		list_del(&t->list);

	if (dst_queue != NULL) {
		list_add_tail(&t->list, dst_queue);
		if (dst_queue == &readyqueue)
			t->state = ST_READY;
		else
			t->state = ST_BLOCKED;
	}
	else {
		t->state = ST_RUN;
	}
}

/* needs_sched_rr - Returns 1 if a context switch is required */
int needs_sched_rr(void)
{
	if (list_empty(&readyqueue))
		return 0;

	/* The idle process must leave the CPU as soon as a process is ready */
	if (current() == idle_task)
		return 1;

	return (ticks_left <= 0);
}

/* sched_next_rr - Selects the next process and performs the context switch */
void sched_next_rr(void)
{
	struct task_struct *next;

	if (!list_empty(&readyqueue)) {
		next = list_head_to_task_struct(list_first(&readyqueue));
		list_del(&next->list);
	}
	else {
		/* There is no ready process: run the idle process */
		next = idle_task;
	}

	next->state = ST_RUN;
	ticks_left = get_quantum(next);

	task_switch((union task_union *)next);
}

void schedule(void)
{
	struct task_struct *cur;

	free_zombies();  

	update_sched_data_rr();

	if (!needs_sched_rr())
		return;

	cur = current();

	if (cur == idle_task) {
		/* The idle process is never inserted into readyqueue */
		idle_task->state = ST_READY;
	}
	else {
		update_process_state_rr(cur, &readyqueue);
	}

	sched_next_rr();
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

/* performs a context change between the current process and the new. 
	new: pointer to the task_union of the process that will be executed*/
void inner_task_switch(union task_union *new)
{
	current()->kernel_esp = (unsigned long *)get_ebp();
	if (current()->PID != new->task.PID){
		set_cr3(new->task.dir_pages_baseAddr); //change the user address space by updating the page directory
	}
	tss.esp0 = KERNEL_ESP(new); //update the system stack pointer to the one of the new task
	set_esp(new->task.kernel_esp);
}
