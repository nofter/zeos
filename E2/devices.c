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

int sys_read_keyboard(char *buf, int count){
    int check;
    current()->info_key.toread = count;
    current()->info_key.buffer = buf;
    if (list_empty(&keyboardqueue)) {  
        if (count <= q) {		//enough keyboard stuff to read
            int tmp = minim(BUFF_SIZE - p, count);
            check = copy_to_user(&keyboardbuffer[p], buf, tmp);
            if (check < 0) return check;
            q -= tmp;
            p = (p + tmp)%BUFF_SIZE;

            check = copy_to_user(&keyboardbuffer[p], &buf[tmp], count - tmp);
            if (check < 0) return check;
            tmp = count - tmp;
            q -= tmp;
            p = (p + tmp)%BUFF_SIZE;

            current()->info_key.toread = 0;
            current()->info_key.buffer =  NULL;
        }
        else {
            while (current()->info_key.toread > 0) {
                int tmp = minim(BUFF_SIZE - p, q);
                tmp = minim(tmp, current()->info_key.toread);
                check = copy_to_user(&keyboardbuffer[p], current()->info_key.buffer, tmp);
                if (check < 0) return check;
                q -= tmp;
                p = (p + tmp)%BUFF_SIZE;
                
                int tmp2 = min(q, current()->info_key.toread - tmp);
                check = copy_to_user(&keyboardbuffer[p], &current()->info_key.buffer[tmp], tmp2);
                if (check < 0) return check;
                tmp += tmp2;
                q = q - tmp;
                p = (p + tmp2)%BUFF_SIZE;
    
                current()->info_key.toread -= tmp;
                current()->info_key.buffer = &(current()->info_key.buffer[tmp]);
	    	    update_process_state_rr(current(), &keyboardqueue);
                sched_next_rr();
            }
        }
    }
    else {
		current()->info_key.buffer = buf;
        current()->info_key.toread = count;
		update_process_state_rr(current(), &keyboardqueue);
        sched_next_rr();
        while (current()->info_key.toread > 0) {
                int tmp = minim(BUFF_SIZE - p, q);
                tmp = minim(tmp, current()->info_key.toread);
                check = copy_to_user(&keyboardbuffer[p], current()->info_key.buffer, tmp);
                if (check < 0) return check;
                q -= tmp;
                p = (p + tmp)%BUFF_SIZE;
                
                int tmp2 = min(q, current()->info_key.toread - tmp);
                check = copy_to_user(&keyboardbuffer[p], &current()->info_key.buffer[tmp], tmp2);
                if (check < 0) return check;
                tmp += tmp2;
                q = q - tmp;
                p = (p + tmp2)%BUFF_SIZE;
    
                current()->info_key.toread -= tmp;
                current()->info_key.buffer = &(current()->info_key.buffer[tmp]);
	    	    update_process_state_rr(current(), &keyboardqueue);
                sched_next_rr();
            }
    }
    return count;
}


int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}
