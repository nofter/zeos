#include <libc.h>
#include <stats.h>
#include<semaphore.h>

// HELPER TEST VARS //
char buff[24];
char buff2[24];
int pid;




int __attribute__ ((__section__(".text.main")))
  main(void)
{

    runjp();

    while(1);
    
    return 0;
}


    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

     //runjp(); while(1);
//int i;
//for (i=0; i<100; i++) write(1, "\n", 2);
/*
int a = fork();
if (a==0){
     runjp(); while(1);
    while(1){int valor2 = getpid();
    itoa(valor2,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);}
}
else{ while(1){int valor2 = getpid();
    itoa(valor2,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);};}
*/
    //int a = fork();    a= fork();
//while(1){
/*    int valor = gettime();
    itoa(valor,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);*/

    /*int valor2 = getpid();
    itoa(valor2,buff2);
    write(1, buff2, sizeof(buff2));
    write(1, "  ", 3);
    write(1, "\n", 2);*/

    /*int valor3 = get_stats(valor2, &st);
    itoa(st.elapsed_total_ticks,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);
    itoa(st.user_ticks,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);
    itoa(st.system_ticks,buff);
    write(1, buff, sizeof(buff));
    write(1, "  ", 3);
    write(1, "\n", 2);*/

    /*if(getpid()==1)
    if (gettime()!=0 && gettime()%100 ==0){
      fork();
    read(1, buff2, 4);
    }*/