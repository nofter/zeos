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
#include <semaphore.h>

#define INITIAL_ESP       	KERNEL_ESP(&task[1])
#define NR_TASKS      10
#define NR_SEMS      10
#define KERNEL_STACK_SIZE	1024
#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

/* Arbitrary value */
#define DEFAULT_QUANTUM 50

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
  page_table_entry * dir_pages_baseAddr;
  struct list_head list;
  int PID;			              /* Process ID. This MUST be the first field of the struct. */
  unsigned long* kernel_esp;  /*"stackpointer(unsigned long)/register(DWord)" type*/
  enum state_t status;
  int total_quantum;		      /* Total quantum of the process */
  struct stats p_stats;		    /* Process stats */
  unsigned int remainder_reads;
  char *heap_break;           /*heap del sbrk*/
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

extern union task_union protected_tasks[NR_TASKS+2];
extern union task_union *task;              /* Vector de tasques */
extern struct task_struct *idle_task;


extern struct list_head freequeue;
extern struct list_head readyqueue;
extern struct list_head keyboardqueue;

extern struct sem_t sems[NR_SEMS];


/* TODO: Would be better to define this in include/mm.h and using
 * __attribute__((__section__(".data.task"))); ?
 */
extern int dir_pages_refs[NR_TASKS];
 
/* Useful macro to manipulates directory pages references */
#define POS_TO_DIR_PAGES_REFS(p_dir)                        \
    (int)(((unsigned long)p_dir - (unsigned long)(&dir_pages[0][0])) / (sizeof(dir_pages[0]))) \
 
/* Inicialitza les dades del proces inicial */

void init_task1(void);
void init_idle(void);

void init_sched(void);
//void init_readyqueue(void);  //TODO move a un altre lloc
//void init_freequeue(void);  //TODO move a un altre lloc

struct task_struct * current();

void task_switch(union task_union*t);
void inner_task_switch(union task_union* inner_new);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);
void update_DIR_refs(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

void schedule();

/* Headers for the scheduling policy */
void sched_next_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
void update_sched_data_rr();
int needs_sched_rr();
void update_current_state_rr(struct list_head *dest);

/* Headers del quantum */
int get_quantum (struct task_struct *t);
void set_quantum (struct task_struct *t, int new_quantum);

/* Headers dels stats */
void init_stats(struct stats *s);
void update_stats(unsigned long *sys_ticks, unsigned long *elapsed);

/* blocking/unblock management functions */
//void block(struct list_head * process, struct list_head * dst_queue);
//void unblock(struct list_head * process);
void block_to_keyboardqueue(int endpoint);
void unblock_from_keyboardqueue();

#endif  /* __SCHED_H__ */
