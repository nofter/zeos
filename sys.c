/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#include <system.h>

#include <stats.h>

#define LECTURA 0
#define ESCRIPTURA 1

int global_PID = 1000;
extern int remaining_quantum;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork(){
  return 0;
}

struct list_head readyqueue;

int sys_fork()
{
    //update_stats(current(), RUSER_TO_RSYS);
    //current()->stats.

    int PID = -1; //SEGURETAT ???
    unsigned int i;

    /* Returns error if there isn't any available task in the free queue */
    if (list_empty(&freequeue)) {
        //update_stats(current(), RSYS_TO_RUSER);
        return -EAGAIN;
    }

    /* Needed variables related to child and parent processes */
    struct list_head *free_pcb = list_first(&freequeue);
    union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
    union task_union *parent = (union task_union *)current();
    struct task_struct *pcb_child = &(child->task);
    struct task_struct *pcb_parent = &(parent->task);

    list_del(free_pcb);

    /* Inherits system code+data */
    copy_data(parent, child, sizeof(union task_union));

    /* Allocates new page directory for child process */
    allocate_DIR(pcb_child);

    page_table_entry* pagt_child = get_PT(pcb_child);
    page_table_entry* pagt_parent = get_PT(pcb_parent);

    /* Reserve free frames (physical memory) to allocate child's user data */
    int resv_frames[NUM_PAG_DATA];

    for (i = 0; i < NUM_PAG_DATA; i++) {

        /* If there is no enough free frames, those reserved thus far must be freed */
        if ((resv_frames[i] = alloc_frame()) == -1) {
            while (i >= 0) free_frame(resv_frames[i--]);
            list_add_tail(&(pcb_child->list), &freequeue);
            //update_stats(current(), RSYS_TO_RUSER);
            return -ENOMEM;
        }
    }

    /* Inherits user code. Since it's shared among child and father, only it's needed
     * to update child's page table in order to map the correpond entries to frames
     * which allocates father's user code.
     */
    for (i = PAG_LOG_INIT_CODE; i < PAG_LOG_INIT_DATA; i++) {
        set_ss_pag(pagt_child, i, get_frame(pagt_parent, i));
    }

    /* Inherits user data. Since each process has its own copy allocated in physical
     * memory, it's needed to copy the user data from parent process to the news
     * reserved frames. First the page table entries from child process must be
     * associated to the new reserved frames. Then the user copy data is performed by
     * modifying the logical adress space of the parent to points to reserved frames,
     * then makes the copy of data, and finally deletes these new entries of parent's
     * page table to deny the access to the child's user data.
     */
    unsigned int stride = PAGE_SIZE * NUM_PAG_DATA;
    for (i = 0; i < NUM_PAG_DATA; i++) {
        /* Associates a logical page from child's page table to physical reserved frame */
        set_ss_pag(pagt_child, PAG_LOG_INIT_DATA+i, resv_frames[i]);

        /* Inherits one page of user data */
        unsigned int logic_addr = (i + PAG_LOG_INIT_DATA) * PAGE_SIZE;
        set_ss_pag(pagt_parent, i + PAG_LOG_INIT_DATA + NUM_PAG_DATA, resv_frames[i]);
        copy_data((void *)(logic_addr), (void *)(logic_addr + stride), PAGE_SIZE);
        del_ss_pag(pagt_parent, i + PAG_LOG_INIT_DATA + NUM_PAG_DATA);
    }


    /* Flushes entire TLB */
    set_cr3(get_DIR(pcb_parent));

    /* Updates child's PCB (only the ones that the child process does not inherit) */
    pcb_child->PID = global_PID++;
    pcb_child->status = ST_READY;
    //pcb_child->remainder_reads = 0;
    //init_stats(pcb_child);

    /* Prepares the return of child process. It must return 0
     * and its kernel_esp must point to the top of the stack
     */
    unsigned int ebp;
    __asm__ __volatile__(
        "mov %%ebp,%0\n"
        :"=g"(ebp)
    );

    unsigned int stack_stride = (ebp - (unsigned int)parent)/sizeof(unsigned long);

    /* Dummy value for ebp for the child process */
    child->stack[stack_stride-1] = 0;

    child->stack[stack_stride] = (unsigned long)&ret_from_fork;
    child->task.kernel_esp = &child->stack[stack_stride-1];

    /* Adds child process to ready queue and returns its PID from parent */
    list_add_tail(&(pcb_child->list), &readyqueue);

    /* If current process is idle, immediately removes from the CPU */
    if (current()->PID == 0) sched_next_rr();

//update_stats(current(), RSYS_TO_RUSER);

    return PID;
}


void sys_exit()
{
    int i;

    /*Aconseguim les taula de pagines del process*/
    page_table_entry *process_PT = get_PT(current());

    /*Free the data structures and resources of this process*/
    for (i = 0; i < NUM_PAG_DATA; i++)
      {
        /*allibarem els phisical_mem del process*/
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        /*Allibarem les pagines logiques del proces*/
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
        }

    /*reset PID*/
    current()-> PID = -1;

    /*alliberem el task_struct de la llista*/
    list_add_tail(&(current()->list), &freequeue);

    /*scheduling*/
    sched_next_rr();
}


int sys_write(int fd, char * buffer, int size)
{
	int nok, noaccess, res;
//fd: file descriptor. In this delivery it must always be 1.
	if(nok = check_fd(fd,ESCRIPTURA)) return nok;
//buffer: pointer to the bytes.
	if(buffer == NULL) return -EFAULT;
//size: number of bytes.
	if (size<0) return -EINVAL;

	//if (noaccess = access_ok(VERIFY_WRITE, buffer, size) return -EFAULT;

	res = sys_write_console(buffer, size);

	return res;
  //return ’ Negative number in case of error (specifying the kind of error) and
  //the number of bytes written if OK.
}

int sys_gettime()
{
	return zeos_ticks;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}


int sys_get_stats(int pid, struct stats *st)
{
  int i;

  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT;

  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}
