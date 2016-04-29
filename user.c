#include <libc.h>
#include <stats.h>

char buff[24];
int pid;
int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    runjp_rank();
    while(1);

    /*  while(1){
        char a [100];
         struct stats stats_t;
        get_stats(getpid(), &stats_t);
        itoa(stats_t.total_trans, a);
        write(1, a, strlen(a));
        write(1, "\n", 1);
      }*/


      	//a = fork();     a= fork();

    //int valor = gettime();
    /*itoa(valor,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);*/

/*    int valor2 = getpid();
    itoa(valor2,buff2);
    write(1, buff2, sizeof(buff2));
    write(1, "  ", 3);

/*    int valor3 = get_stats(valor2, &st);
    itoa(st.elapsed_total_ticks,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);
    itoa(st.user_ticks,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);
    itoa(st.system_ticks,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);
    write(1, "\n", 2);
*/
    /*if(getpid()==1)
    if (gettime()!=0 && gettime()%1000 ==0){
      fork();
    }*/
    //task_switch((union task_union*)idle_task);
    //else task_switch((union task_union*)task1);

}
