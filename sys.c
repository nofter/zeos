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

#include <sys.h>

#include <stats.h>

#include<semaphore.h>


#define LECTURA 0
#define ESCRIPTURA 1

// VARS //

int global_PID = 1000;
extern int remaining_quantum;


// SYSCALL DECLARATIONS (avoiding warnings) //

int sys_sem_destroy(int n_sem);

// HELPERS //

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}


// SYSCALLS //

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

// TODO - Clean
//struct list_head readyqueue;

int sys_fork()
{
// TODO - Delete printk
//printk("\nfork...");

    user_to_system();
    unsigned int i;

    /* Returns error if there isn't any available task in the free queue */
    if (list_empty(&freequeue)) return -ENOMEM;

    /* Needed variables related to child and parent processes */
    struct list_head *free_pcb = list_first(&freequeue);
    union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
    struct task_struct *pcb_child = &(child->task);

    list_del(free_pcb);

    /* Inherits system code+data */
    copy_data(current(), child, sizeof(union task_union));

    /* Allocates new page directory for child process */
    allocate_DIR(pcb_child);

    page_table_entry* pagt_child = get_PT(&child->task);
    page_table_entry* pagt_parent = get_PT(current());

    /* Reserve free frames (physical memory) to allocate child's user data */
    int resv_frames[NUM_PAG_DATA];

    for (i = 0; i < NUM_PAG_DATA;  i++) {

        /* If there is no enough free frames, those reserved thus far must be freed */
        if ((resv_frames[i] = alloc_frame()) == -1) {
            while (i >= 0) free_frame(resv_frames[i--]);
            list_add_tail(free_pcb, &freequeue);
            system_to_user();
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
    set_cr3(get_DIR(current()));

    /* Updates child's PCB (only the ones that the child process does not inherit) */
    child->task.PID = ++global_PID;
    init_stats(&(child->task.p_stats));
    child->task.status = ST_READY;

    /* Prepares the return of child process. It must return 0
     * and its kernel_esp must point to the top of the stack
     */
    unsigned int ebp;
    __asm__ __volatile__(
        "mov %%ebp,%0\n"
        :"=g"(ebp)
    );

    unsigned int stack_stride = (ebp - (unsigned int)current())/sizeof(unsigned long);

    /* Dummy value for ebp for the child process */
    child->stack[stack_stride-1] = 0;

    child->stack[stack_stride] = (unsigned long)&ret_from_fork;
    child->task.kernel_esp = &child->stack[stack_stride-1];

    /* Adds child process to ready queue and returns its PID from parent */
    list_add_tail(&(pcb_child->list), &readyqueue);

    /* If current process is idle, immediately removes from the CPU */
    if (current()->PID == 0) sched_next_rr();

    system_to_user();
    return child->task.PID;
}

int sys_clone(void (*function) (void), void *stack)
{
// TODO - Delete printk
//printk("\nclone...");

     user_to_system();

    /* Checks user parameters */
    if (!access_ok(VERIFY_READ, function, sizeof(function)) || !access_ok(VERIFY_WRITE, stack, sizeof(stack))) {
        system_to_user();        
         return -EFAULT;
    }
 
    /* Returns error if there isn't any available task in the free queue */
    if (list_empty(&freequeue)) {
        system_to_user();
        return -EAGAIN;
    }

    /* Needed variables related to child and parent processes */
    struct list_head *free_pcb = list_first(&freequeue);
    union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
    union task_union *parent = (union task_union *)current();
    struct task_struct *pcb_child = &(child->task);

    list_del(free_pcb);

    /* Inherits system code+data */
    copy_data(parent, child, sizeof(union task_union));
 
    /* Updates references for child's page directory, inherited by parent */
    update_DIR_refs(pcb_child);
 
    /* Updates child's PCB (only the ones that the child process does not inherit) */
   
    pcb_child->PID = ++global_PID;
    pcb_child->status = ST_READY;
 
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
 
    /* Modifies ebp with the address of the new stack */
    child->stack[stack_stride+7] = (unsigned long)stack;
 
    /* Modifies eip with the address of the new code (function) to execute */
    child->stack[stack_stride+13] = (unsigned long)function;
 
    /* Modifies esp with the address of the new stack */
    child->stack[stack_stride+16] = (unsigned long)stack;
 
    /* Adds child process to ready queue and returns its PID from parent */
    list_add_tail(&(pcb_child->list), &readyqueue);
 
    /* If current process is idle, immediately removes from the CPU */
    if (current()->PID == 0) sched_next_rr();
 
    system_to_user();

    return child->task.PID;
}

void sys_exit()
{
// TODO - Delete printk
//printk("\nexit...");

    int i;

// TODO - CHECK IF (invariant - do not free process PCB nor memory if cloned)
if (--dir_pages_refs[POS_TO_DIR_PAGES_REFS(get_DIR(current()))]==0)
{

    /*Aconseguim la taula de pagines del process*/
    page_table_entry *process_PT = get_PT(current());

    /*Free the data structures and resources of this process*/
    for (i = 0; i < NUM_PAG_DATA; i++)
    {
        /*allibarem els phisical_mem del process*/
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        /*Allibarem les pagines logiques del proces*/
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
     }

// TODO  - CHECK BEHAVIOUR (if cloned)
    for (i = 0; i < NR_SEMS; i++) 
    {
        if (sems[i].owner_pid == current()->PID) {
            sys_sem_destroy(i);
        }
    }
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
	int nok/*, noaccess*/, res;
//fd: file descriptor. In this delivery it must always be 1.
	if((nok = check_fd(fd,ESCRIPTURA))) return nok;
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
// TODO - Delete printk
//printk("\ngettime...");

    user_to_system();
    system_to_user();
	return zeos_ticks;
}


int sys_get_stats(int pid, struct stats *st)
{
// TODO - Delete printk
//printk("\nget_stats...");

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


int sys_sem_init(int n_sem, unsigned int value)
{
// TODO - Delete printk
//printk("\nsem_init...");
    user_to_system();

    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS) {
        system_to_user();
        return -EINVAL;
    }

    if (sems[n_sem].owner_pid != -1) {
        system_to_user();
        return -EBUSY;
    }

    sems[n_sem].owner_pid = current()->PID;
    sems[n_sem].count = value;
    INIT_LIST_HEAD(&(sems[n_sem].semqueue));
    system_to_user();

    return 0;
}


int sys_sem_wait(int n_sem)
{
    user_to_system();

    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS || sems[n_sem].owner_pid == -1) {
        system_to_user();
        return -EINVAL;
    }

    if (sems[n_sem].count > 0){
        --sems[n_sem].count;
    }else {
        struct list_head *semqueue = &(sems[n_sem].semqueue);
        struct list_head *curr_task = &(current()->list);
// TODO - UNCOMMENTING THIS GENERATES A PAGE FAULT
	//printk("\nsem_waitA...");
	//list_del(curr_task);
	//printk("\nsem_waitB...");
        current()->status = ST_BLOCKED;
        list_add_tail(curr_task, semqueue);
        system_to_user();
        sched_next_rr();
    }

    /* Assures that the semaphore was destroyed while the process is blocked */
    if (sems[n_sem].owner_pid == -1) {
        system_to_user();
        return -EPERM;
    }

    system_to_user();
    return 0;
}


int sys_sem_signal(int n_sem)
{
// TODO - Delete printk
//printk("\nsem_signal...");

     user_to_system();

    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS || sems[n_sem].owner_pid == -1) {
        system_to_user();
        return -EINVAL;
    }

    struct list_head *semqueue = &(sems[n_sem].semqueue);

    if (list_empty(semqueue)){

        ++sems[n_sem].count;

    }else {

        struct list_head *elem = list_first(semqueue);
        struct task_struct *unblocked = list_head_to_task_struct(elem);
        list_del(elem);
        unblocked->status = ST_READY;
        list_add_tail(elem, &readyqueue);

        system_to_user();
    }

    system_to_user();
    return 0;
}

int sys_sem_destroy(int n_sem)
{
// TODO - Delete printk
//printk("\ndestroy...");
    user_to_system();

    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS || sems[n_sem].owner_pid == -1) {
        system_to_user();
        return -EINVAL;
    }

    /* Check permisions */
    if (sems[n_sem].owner_pid != current()->PID) {
        system_to_user();
        return -EPERM;
    }

    sems[n_sem].owner_pid = -1;
    struct list_head *semqueue = &(sems[n_sem].semqueue);
    while(!list_empty(semqueue)) {
        struct list_head *elem = list_first(semqueue);
        struct task_struct *unblocked = list_head_to_task_struct(elem);
        list_del(elem);
        unblocked->status = ST_READY;
        list_add_tail(elem, &readyqueue);
        system_to_user();
    }

    system_to_user();
    return 0;
}

