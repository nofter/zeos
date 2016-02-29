#include <libc.h>

char buff[24];
char timer;
int pid;
int i = 0;
int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  while(1) { 

  	int valor = gettime();

  //	write(1, buff , sizeof(buff) ); 
  }
}
