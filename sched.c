/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

void task_switch(union task_union* new){
	//Save ESI, EDI, EBX
	__asm__ __volatile__(
	"pushl %%esi;\n\t"
	"pushl %%edi;\n\t"
	"pushl %%ebx;\n\t"
	: : );

	inner_task_switch(new); //TODO ARREGLAR

	//Restore EBX, EDI, ESI
	__asm__ __volatile__(
	"popl %%ebx;\n\t"
	"popl %%edi;\n\t"
	"popl %%esi;\n\t"
	: : );
}

void inner_task_switch(union task_union* inner_new){
	unsigned long* par;


	//Update TSS
	tss.esp0 = KERNEL_ESP((union task_union*)inner_new); //necessari el casting ???
	//change user address space
	set_cr3(get_DIR(/*(struct task_struct*) */inner_new));

	//store EBP (address of current system stack, where inner_task_switch begins - dynamic link) in PCB
/*	__asm__ __volatile__(
  	"mov %%ebp, %0\n"
	: "=g" (par)
	);
	current()->kernel_esp = par;

	//CHANGE STACK => set ESP to point to the stored value in NEW PCB
	par = ((struct task_struct*)inner_new)->kernel_esp;
	__asm__ __volatile__ (
	"movl %0,%%esp\n"
	: /*no output*/
/*	: "m" (par));

	//restore EBP
	par = current()->kernel_esp;
	__asm__ __volatile__ (
	"movl %0,%%ebp\n"
	"addl $4, %%esp\n"
	: /*no output*/
/*	: "m" (par));

*/	__asm__ __volatile__(
	  	"mov %%ebp, %0\n"
		"movl %1,%%esp\n"
		"popl %%ebp\n"
		"ret\n"
	: "=g" (current()->kernel_esp)
	: "r" (inner_new.task->kernel_esp)
	);

//	return;//RET
}

//#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
//#endif

struct list_head blocked;
struct list_head freequeue;
struct list_head ready_queue;

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


int allocate_DIR(struct task_struct *t)
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_freequeue(void)
{
	INIT_LIST_HEAD(&freequeue);
	int i;

	for(i=0; i < NR_TASKS; i++)
	{
		list_add( &task[i].task.list, &freequeue );
	}
}

void init_readyqueue(void)
{
	INIT_LIST_HEAD(&ready_queue);
}

void init_idle (void)
{
	/*available task_union*/
    struct list_head *first = list_first(&freequeue);
    struct task_struct* idle_task = list_head_to_task_struct(first);
    list_del(first);
    /*Assign PID 0 to the process*/
    idle_task->PID = 0;
    /*Initialize field dir_pages_baseAaddr*/
    allocate_DIR(idle_task);
    /*Initialize an execution context for the procees*/
    union task_union *idle_task_stack = (union task_union *)idle_task;
    /*Store in the stack of the idle process the address of cpu_idle function*/
    idle_task_stack->stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle;
    /*we want to assign to register ebp when undoing the dynamic link (it can be 0),*/
    idle_task_stack->stack[KERNEL_STACK_SIZE-2] = 0;
    /*keep (in a field of its task_struct) the position of the stack where
we have stored the initial value for the ebp register*/
    idle_task->kernel_esp = (unsigned long *)&(idle_task_stack->stack[KERNEL_STACK_SIZE-2]);
}

void init_task1(void)
{
	/*available task_union*/
	struct list_head *first = list_first(&freequeue);
  struct task_struct* PCB_task1 = list_head_to_task_struct(first);
  list_del(first);
  /*Assign PID 1*/
  PCB_task1->PID = 1;
  /*Initialize field dir_pages_baseAaddr*/
  allocate_DIR(PCB_task1);
  /*initialization of its address space*/
  //free_user_pages(PCB_task1);	//COMMENT
  set_user_pages(PCB_task1);
  /*Update the TSS to make it point to the new_task system stack*/
	tss.esp0 = KERNEL_ESP((union task_union*)PCB_task1);
	/*Set its page directory as the current page directory in the system*/
	set_cr3(get_DIR(PCB_task1));

}


void init_sched(){
	init_freequeue();
	init_readyqueue();

}

struct task_struct* current()
{
  int ret_value;

  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}
