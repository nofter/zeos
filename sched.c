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

#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;
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
    idle_task = list_head_to_task_struct(first);
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
	struct list_head *first = list_first(&freequeue);
    task1 = list_head_to_task_struct(first);
    list_del(first);
    /*Assign PID 1*/
    idle_task->PID = 1;
    /*Initialize field dir_pages_baseAaddr*/
    allocate_DIR(task1);
    /*Initialize an execution context for the procees*/
    union task_union *task1_stack = (union task_union *)task1_task;
    
    
}


void init_sched(){

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

