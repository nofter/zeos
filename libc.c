/*
 * libc.c
 */

#include <libc.h>

#include <types.h>

#include <errno.h>


int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;

  if (a==0) { b[0]='0'; b[1]=0; return ;}

  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }

  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}


int strlen(char *a)
{
  int i;

  i=0;

  while (a[i]!=0) i++;

  return i;
}


void exit()
{
  int result;

    __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (result)
    : "a" (1));
    if (result<0){
      errno = -result;
    } else {
      errno = 0;
    }
}


int fork()
{
  int result;

    __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (result)
    : "a" (2));
    if (result<0){
      errno = -result;
      return -1;
    } else {
      errno = 0;
      return result;
    }
}


int read(int fd, char *buf, int count)
{
  int result;

    __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (result)
    : "a" (3), "b" (fd), "c" (buf), "d" (count));
    if (result<0){
      errno = -result;
      return -1;
    } else {
      errno = 0;
      return result;
    }

}

int write(int fd, char *buffer, int size)
{
  int result;

    __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (result)
    : "a" (4), "b" (fd), "c" (buffer), "d" (size));
    if (result<0){
      errno = -result;
      return -1;
    } else {
      errno = 0;
      return result;
    }

}


int gettime()
{
  int result;

    __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (result)
    : "a" (10));
    if (result<0){
      errno = -result;
      return -1;
    } else {
      errno = 0;
      return result;
    }

}


int clone(void (*function) (void), void *stack)
{
  int result;
  __asm__ __volatile__ (
  	"int $0x80\n\t"
	:"=a" (result)
	:"a" (19), "b" (function), "c" (stack) );
  if (result<0)
  {
    errno = -result;
    return -1;
  }
  errno=0;
  return result;
}


int getpid()
{
  int result;

    __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (result)
    : "a" (20));
    if (result<0){
      errno = -result;
      return -1;
    } else {
      errno = 0;
      return result;
    }
}


int sem_init(int n_sem, unsigned int value)
{
  int result;
  __asm__ __volatile__ (
  	"int $0x80\n\t"
	:"=a" (result)
	:"a" (21), "b" (n_sem), "c" (value) );
  if (result<0)
  {
    errno = -result;
    return -1;
  }
  errno=0;
  return result;
}


int sem_wait(int n_sem)
{
  int result;
  __asm__ __volatile__ (
  	"int $0x80\n\t"
	:"=a" (result)
	:"a" (22), "b" (n_sem) );
  if (result<0)
  {
    errno = -result;
    return -1;
  }
  errno=0;
  return result;
}


int sem_signal(int n_sem)
{
  int result;
  __asm__ __volatile__ (
  	"int $0x80\n\t"
	:"=a" (result)
	:"a" (23), "b" (n_sem) );
  if (result<0)
  {
    errno = -result;
    return -1;
  }
  errno=0;
  return result;
}


int sem_destroy(int n_sem)
{
  int result;
  __asm__ __volatile__ (
  	"int $0x80\n\t"
	:"=a" (result)
	:"a" (24), "b" (n_sem) );
  if (result<0)
  {
    errno = -result;
    return -1;
  }
  errno=0;
  return result;
}


int get_stats(int pid, struct stats *st)
{
  int result;
  __asm__ __volatile__ (
  	"int $0x80\n\t"
	:"=a" (result)
	:"a" (35), "b" (pid), "c" (st) );
  if (result<0)
  {
    errno = -result;
    return -1;
  }
  errno=0;
  return result;
}


/* Wrapper for the system call *sys_sbrk(int increment).
 * It has got the entry 15 (0x0F) in the system call table
 */
void *sbrk(int increment)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (increment), "a" (0x0F)
    );

    SET_ERRNO_RETURN
}


void perror()
{
  //
}
