#include <libc.h>

char buff[24];
int pid;
int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  while(1) {
//    int i = 0;
    int valor = gettime();
    itoa(valor,buff);
    write(1, buff, sizeof(buff));
    write(1, "\n", 2);

    valor = getpid();
    itoa(valor,buff);
    write(1, buff, sizeof(buff));

    //if(current()->PID==1)
    //if (gettime()!=0 && gettime()%1000 ==0){
    //  fork();
    //}
    //task_switch((union task_union*)idle_task);
    //else task_switch((union task_union*)task1);

  }
}
