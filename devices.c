#include <io.h>
#include <utils.h>
#include <list.h>

#include<sched.h>
#include<sys.h>

// Queue for blocked processes in I/O 
struct list_head blocked;

int minim(int a, int b) {
    if (a <= b) return a;
    return b;
}


int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}
