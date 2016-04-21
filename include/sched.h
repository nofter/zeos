/*
 * sched.h - Estructures i macros pel tractament de processos
*/
#ifndef __SCHED_H__

#define __SCHED_H__

#include <list.h>

#include <types.h>
#include <mm_address.h>
#include <stats.h>
#include <utils.h>


#define INITIAL_ESP       	KERNEL_ESP(&task[1])
#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024
#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]


enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
  page_table_entry * dir_pages_baseAddr;
  struct list_head list;
  int PID;			/* Process ID. This MUST be the first field of the struct. */
  unsigned long* kernel_esp;		//"stackpointer(unsigned long)/register(DWord)" type
//int quantum;
  enum state_t status;
  int total_quantum;		/* Total quantum of the process */
  struct stats p_stats;		/* Process stats */
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

extern union task_union protected_tasks[NR_TASKS+2];
extern union task_union *task; /* Vector de tasques */
extern struct task_struct *idle_task;

extern struct list_head blocked;
extern struct list_head freequeue;
extern struct list_head ready_queue;



/* Inicialitza les dades del proces inicial */

void init_task1(void);
void init_idle(void);

void init_sched(void);
void init_readyqueue(void);  //TODO move a un altre lloc
void init_freequeue(void);  //TODO move a un altre lloc

struct task_struct * current();

void task_switch(union task_union*t);
void inner_task_switch(union task_union* inner_new);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
void update_sched_data_rr();
int needs_sched_rr();

/*headers del quantum*/
int get_quantum (struct task_struct *t);
void set_quantum (struct task_struct *t, int new_quantum);

/*Headers dels stats*/
void init_stats(struct stats *s);
void update_stats(unsigned long *sys_ticks, unsigned long *elapsed);
#endif  /* __SCHED_H__ */
