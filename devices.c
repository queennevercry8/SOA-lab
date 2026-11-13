#include <io.h>
#include <utils.h>
#include <list.h>


// Queue for blocked processes in I/O 
struct list_head blocked;
static struct buffer_circular cajita;


int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}

void init_buffer_circular(void){
  cajita.idx_lec = 0;
  cajita.idx_esc = 0;
  cajita.n_char = 0;
  INIT_LIST_HEAD(&blocked);
}