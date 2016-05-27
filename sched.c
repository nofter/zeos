/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

int remaining_quantum = 0;
int dir_pages_refs[NR_TASKS] = {0};

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

extern struct list_head blocked;
struct list_head freequeue;
struct list_head readyqueue;
struct list_head keyboardqueue;

struct task_struct *idle_task;

struct sem_t sems[NR_SEMS];

int p, q;

void init_stats(struct stats *s)
{
	s->user_ticks = 0;
	s->system_ticks = 0;
	s->blocked_ticks = 0;
	s->ready_ticks = 0;
	s->elapsed_total_ticks = get_ticks();
	s->total_trans = 0;
	s->remaining_ticks = get_ticks();
}

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
	//unsigned long* par;

	//Update TSS
	tss.esp0 = KERNEL_ESP(inner_new); //necessari el casting ???
	//change user address space
	set_cr3(get_DIR(/*(struct task_struct*)*/ &inner_new->task)); //COMMENT cast necessari ??? al codi d'examen no hi ha cast

	//store EBP (address of current system stack, where inner_task_switch begins - dynamic link) in PCB
	__asm__ __volatile__(
  	"mov %%ebp, %0\n\t"
	: "=g" (current()->kernel_esp)
	);
	//current()->kernel_esp = par;

	//CHANGE STACK => set ESP to point to the stored value in NEW PCB
	//par = ((struct task_struct*)inner_new)->kernel_esp;
	__asm__ __volatile__ (
	"movl %0,%%esp\n\t"
	: /*no output*/
	: "m" (inner_new->task.kernel_esp)
);
	//restore EBP
	/*par = current()->kernel_esp;
	__asm__ __volatile__ (
	"movl %0,%%ebp\n"
	"addl $4, %%esp\n"
	:
	: "m" (par));*/
  __asm__ __volatile__ (
    "popl %%ebp\n\t"
    :
    : );

  __asm__ __volatile__ (
    "ret\n\t"
    :
    : );

}

//#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
//#endif

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
    for (pos = 0; pos < NR_TASKS; pos++) {
        if (dir_pages_refs[pos] == 0) {
            ++dir_pages_refs[pos];
            t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];
            return 1;
        }
    }
    return -1;
}
 
void update_DIR_refs(struct task_struct *t)
{
    /* Calculates which directory page entry has assigned */
    ++dir_pages_refs[POS_TO_DIR_PAGES_REFS(get_DIR(t))];
}

/*
int allocate_DIR(struct task_struct *t)
{


	//TODO afgir comptabilitat d'usos per PCBs / vector de llibertat com la llista de frames lliures + comptador de "propietaris" (es pot implementar en un sol vector amb enters)
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];

	return 1;
}
*/

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{

;
	}
}


void init_freequeue()
{
	INIT_LIST_HEAD(&freequeue);
	int i;

	for(i=0; i < NR_TASKS; i++)
	{
    task[i].task.PID = -1;
		list_add( &(task[i].task.list), &freequeue );
	}
}


void init_readyqueue(void)
{
	INIT_LIST_HEAD(&readyqueue);
}

void init_sems()
{
    int i;
    for (i = 0; i < NR_SEMS; i++) {
        sems[i].id = i;
        sems[i].count = 0;
        sems[i].owner_pid = -1;
    }
}

void init_keyboard(void) {
    INIT_LIST_HEAD(&keyboardqueue);
}

int get_quantum (struct task_struct *t)
{
	return t->total_quantum;
}

void set_quantum(struct task_struct *t, int new_quantum)
{
  t->total_quantum = new_quantum;
}


void init_idle (void)
{
	/*available task_union*/
    struct list_head *first = list_first(&freequeue);
    struct task_struct* i_task = list_head_to_task_struct(first);
    list_del(first);
    /*Assign PID 0 to the process*/
    i_task->PID = 0;
    /*Initialize field dir_pages_baseAaddr*/
    allocate_DIR(i_task);
    /*Initialize an execution context for the procees*/
    union task_union *i_task_stack = (union task_union *)i_task;
    /*Store in the stack of the idle process the address of cpu_idle function*/
    i_task_stack->stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle;
    /*we want to assign to register ebp when undoing the dynamic link (it can be 0),*/
    i_task_stack->stack[KERNEL_STACK_SIZE-2] = 0;
    /*keep (in a field of its task_struct) the position of the stack where
    we have stored the initial value for the ebp register*/
    i_task->kernel_esp = (unsigned long *)&(i_task_stack->stack[KERNEL_STACK_SIZE-2]);
    idle_task = i_task;
}

void init_task1(void)
{
	/*available task_union*/
	struct list_head *first = list_first(&freequeue);
  list_del(first);
  struct task_struct *task1 = list_head_to_task_struct(first);
  union task_union *tu1 = (union task_union*)task1;
  /*Assign PID 1*/
  task1->PID = 1;
  task1->status=ST_RUN;
  set_quantum(task1, DEFAULT_QUANTUM);
  task1->heap_break = (unsigned long *)(HEAPSTART * PAGE_SIZE);
  remaining_quantum = get_quantum(task1);
  init_stats(&task1->p_stats);
  /*Initialize field dir_pages_baseAaddr*/
  allocate_DIR(task1);
  /*initialization of its address space*/
  set_user_pages(task1);
  /*Update the TSS to make it point to the new_task system stack*/
  tss.esp0=(DWord)&(tu1->stack[KERNEL_STACK_SIZE]);
	/*Set its page directory as the current page directory in the system*/
//set_cr3(get_DIR(task1));
set_cr3(task1->dir_pages_baseAddr);
}

void init_sched()
{
	init_freequeue();
	init_readyqueue();
	init_sems(); //Initializes array of semaphores
	init_keyboard();
  remaining_quantum = DEFAULT_QUANTUM;
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

void update_stats(unsigned long *sys_ticks, unsigned long *elapsed)
{
  unsigned long current_ticks;

  current_ticks=get_ticks();

  *sys_ticks += current_ticks - *elapsed;

  *elapsed=current_ticks;

}

void schedule()
{
  update_sched_data_rr();
  if (needs_sched_rr())
  {
    update_process_state_rr(current(), &readyqueue);
    sched_next_rr();
  }
}

void update_sched_data_rr(void)
{
  remaining_quantum--;
}

int needs_sched_rr(void)
{
  if ((remaining_quantum==0)&&(!list_empty(&readyqueue))) return 1;
  if (remaining_quantum==0){
     remaining_quantum = get_quantum(current());
     current()->p_stats.total_trans++;
  }
  return 0;
}


void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
  if (t->status!=ST_RUN) list_del(&(t->list));
  if (dst_queue!=NULL)
  {
    list_add_tail(&(t->list), dst_queue);
    if (dst_queue!=&readyqueue) t->status=ST_BLOCKED;
    else
    {
      update_stats(&(t->p_stats.system_ticks), &(t->p_stats.elapsed_total_ticks));
      t->status=ST_READY;
    }
  }
  else t->status=ST_RUN;
}

void update_current_state_rr(struct list_head *dst_queue)
{
    /* Updates the state of current process */
    struct task_struct *pcb_curr_task = current();
//    if (dst_queue == &freequeue) pcb_curr_task->status = ST_FREE;
    /*else*/ if (dst_queue == &readyqueue) pcb_curr_task->status = ST_READY;
    else pcb_curr_task->status = ST_BLOCKED;

    /* Removes current process from its current queue and put it to dst_queue
     * only if the current process is not the idle process and it's not the only
     * available process which status is ready.
     */
    if ((pcb_curr_task != idle_task) & (!list_empty(&readyqueue))) {
        list_del(&(pcb_curr_task->list));
        list_add_tail(&(pcb_curr_task->list), dst_queue);
    }
}

void sched_next_rr(void)
{
  struct list_head *e;
  struct task_struct *t;

  if (!list_empty(&readyqueue)) {
	   e = list_first(&readyqueue);
     list_del(e);

     t=list_head_to_task_struct(e);
  }
  else t=idle_task;

  t->status=ST_RUN;
  remaining_quantum = get_quantum(t);

  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
  update_stats(&(t->p_stats.ready_ticks), &(t->p_stats.elapsed_total_ticks));
  t->p_stats.total_trans++;

  task_switch((union task_union*)t);
}

/*void block(struct list_head * process, struct list_head * dst_queue) {
    list_add_tail(process, dst_queue);
    sched_next_rr();
}

void unblock(struct list_head * process) {
    list_del(process);
    list_add(process, &readyqueue);
}*/

/* endpoint determines if the current process will block at the beginning
 * of the keyboardqueue (0) or at the end (1)
 */
void block_to_keyboardqueue(int endpoint) {

    /* Blocks the current process at the end of the keyboardqueue */
    if (endpoint == 1) {
        update_current_state_rr(&keyboardqueue);
    }

    /* Blocks the current process at the beginning of the keyboardqueue */
    else {
        struct task_struct *pcb_curr_task = current();
        pcb_curr_task->status = ST_BLOCKED;
        if ((pcb_curr_task != idle_task) & (!list_empty(&readyqueue))) {
            list_del(&(pcb_curr_task->list));
            list_add(&(pcb_curr_task->list), &keyboardqueue);
        }
    }
    sched_next_rr();
}

void unblock_from_keyboardqueue() {
    struct list_head *first = list_first(&keyboardqueue);
    struct task_struct *task_first = list_head_to_task_struct(first);

    task_first->status = ST_READY;
    list_del(first);
    list_add_tail(first, &readyqueue);
    if (needs_sched_rr()) sched_next_rr();
}


