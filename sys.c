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
 
#define LECTURA 0
#define ESCRIPTURA 1

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

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

void sys_exit()
{  
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

struct task_struct *idle_task;
int sys_gettime()
{
	//if (zeos_ticks%10 < 0) task_switch((union task_union*) idle_task); //TODO
	return zeos_ticks;
}
