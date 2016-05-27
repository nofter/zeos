/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <system.h>
#include <sys.h>
#include <charmap.h>
#include <keyboard.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

char char_map[98]/* =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','�','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','�',
  '\0','�','\0','�','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
}*/;

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;

  set_handlers();

  /*interruptions*/
  setInterruptHandler(32, clock_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);


  /* system call*/
  setTrapHandler(0x80, system_call_handler, 3);

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */

  set_idt_reg(&idtR);
}


/*void IO_keyboard_mgmt(char c) {
// TODO estalviarse afegir tecla si keyboardqueue esta buida ¿?
  if (BUFF_SIZE > q) {  //hi ha espai al buffer circular -> introduir lletra
    keyboardbuffer[(p + q)%BUFF_SIZE] = c;
    ++q;
	printc_xy(10, 10, c);
  }  //else {la lletra introduida es perd}
  if (!list_empty(&keyboardqueue)) {  //process esperant input
    struct list_head * lh = list_first(&keyboardqueue);
    struct task_struct *tsk = list_head_to_task_struct(lh);
    tsk->status = ST_READY;
    list_del(lh);        
    list_add_tail(lh, &readyqueue);
  }
}*/


// OLD KEYBOARD ROUTINE - TODO Delete
/*void keyboard_routine()
{
  Byte lletra;
  lletra = inb(0x60);
  if (lletra & 0x80) printc_xy(10, 10, char_map[lletra&0x7f]);
}*/

void keyboard_routine()
{
    //update_stats(current(), RUSER_TO_RSYS);
    user_to_system();

    unsigned char key = inb(0x60);
    if (key < 0x80) {
        key = char_map[key];

        int avail_keys = keyboard_buffer_avail();
        if (avail_keys < KBD_BUFFER_SIZE) {
            push_keyboard_buff(key);
            avail_keys = keyboard_buffer_avail();
        }
        if (!list_empty(&keyboardqueue)) {
            struct list_head *first = list_first(&keyboardqueue);
            struct task_struct *task_to_unblock = list_head_to_task_struct(first);
            int last_read_req = task_to_unblock->remainder_reads;
            if (last_read_req <= avail_keys || avail_keys == KBD_BUFFER_SIZE) {
                unblock_from_keyboardqueue();
            }
        }
    }
    //update_stats(current(), RSYS_TO_RUSER);
    system_to_user();
}



void clock_routine()
{
  user_to_system();
  zeos_ticks++;
  zeos_show_clock();
  schedule();
  system_to_user();
}
