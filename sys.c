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

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;
int global_pid;

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

//-----------------------------------------------------------------------
int sys_write(int *buff)
{
  int fd;
  char *buffer;
  int size;

 
  if (access_ok(VERIFY_READ, buff, 12) == 0) return -14; /* EFAULT */

  int parametros[3];
  if (copy_from_user(buff, parametros, 12) < 0) return -14; /* EFAULT */
  
  fd = parametros[0];
  buffer = (char *)parametros[1];
  size = parametros[2];

 
  int ret = check_fd(fd, ESCRIPTURA);
  if (ret < 0) return ret;

  if(buffer == NULL) return -14; /*EFAULT*/  
  if (size < 0) return -22;    // -EINVAL (Invalid argument)
  if (size == 0) return 0;

  //existe mapeo a dirección física?
  if(access_ok(VERIFY_READ, buffer, size) == 0) return -14; /*EFAULT*/

  char kernel_buffer[1024];
  int written = 0;
  int left = size;
  while(left > 0){
    int chunk_size = (left > 1024) ? 1024 : left;
    if (copy_from_user(buffer + written, kernel_buffer, chunk_size) < 0) {
        return -14; /* EFAULT */
    }

    int escrito = sys_write_console(kernel_buffer, chunk_size);
    if(escrito < 0) return escrito;

    written += chunk_size;
    left -= chunk_size;

  }

  return written;
}

int sys_gettime(){
  return zeos_ticks;
}

int sys_getpid(void){
  return current()->PID;
}


int sys_fork(void){ 

  page_table_entry *sysPT = (page_table_entry *) (current()->dir_pages_baseAddr[0].bits.pbase_addr << 12);

  //1. Allocate frame for task struct
  int taskFrameID = alloc_frame();
  if(taskFrameID == -1) return -ENOMEM;
  set_ss_pag(sysPT, taskFrameID, taskFrameID, 0); 
  union task_union *childunion = (union task_union *) (taskFrameID << 12);
  
  //inherit system data by copying the parent's task_union
  copy_data(current(), childunion, sizeof(union task_union));

  //2. Allocate frame for page directory
  int PDframeID = alloc_frame();
  if(PDframeID == -1 ){
    del_ss_pag(sysPT, taskFrameID);
    free_frame(taskFrameID);
    return -ENOMEM;
  }
  set_ss_pag(sysPT, PDframeID, PDframeID, 0);
  page_table_entry *PageDir = (page_table_entry *) (PDframeID << 12);
  clear_page_table(PageDir);

  //2.1.Shared System PT with the child + assign the child's PCB his own Page Directory
  
  /* ALTERNATIVA DIAPOSITIVAS
    int childPTSframe = alloc_frame();
    if (childPTSframe == -1){
      errno = 12;
      return -1;
    }
    page_table_entry *ChildPT = (page_table_entry *)(childPTSframe << 12);
    clear_page_table(ChildPT);
    PageDir[0].entry = (current()->dir_pages_baseAddr[0].entry;
    page_table_entry *ParentPT = get_PT(current());

    int frameID;
    for (frameID = KERNEL_STAR_PAGE; frameID <= KERNEL_END_PAGE; frameID++){
        // Obtain the physical frame associated to a logical frame
        int map = get_frame(ParentPT, frameID);
        // Map the frame in the same entry of the child’s PT
        set_ss_frame(ChildPT, frameID, map);
    }
  
  */
  
  PageDir[0].entry = current()->dir_pages_baseAddr[0].entry;

  //2.2. Inherit user data
  int childPTframe = alloc_frame();
  if(childPTframe == -1){
    del_ss_pag(sysPT, taskFrameID);
    del_ss_pag(sysPT, PDframeID);
    free_frame(taskFrameID);
    free_frame(PDframeID);
    return -ENOMEM;
  }
  set_ss_pag(sysPT, childPTframe, childPTframe, 0);
  page_table_entry *childPTU = (page_table_entry *)(childPTframe << 12);
  clear_page_table(childPTU);
  
  set_ss_pag(PageDir, 1, childPTframe, 1);
  childunion->task.dir_pages_baseAddr = PageDir;

  page_table_entry *parentPTU = (page_table_entry *)(current()->dir_pages_baseAddr[1].bits.pbase_addr << 12);  
  //2.2.1.Map user code
  for(int i = NUM_PAG_DATA; i < NUM_PAG_DATA + NUM_PAG_CODE; i++){
    set_ss_pag(childPTU, i, get_frame(parentPTU, i), 1 );
    childPTU[i].bits.rw = 0; //only read just in case
  }


  //2.2.2. Map data + stack in new frames
  int tmpmapping = NUM_PAG_CODE + NUM_PAG_DATA;
  int frames[NUM_PAG_DATA];

  for(int i = 0; i < NUM_PAG_DATA; i++){
    frames[i] = alloc_frame(); 
    if(frames[i] == -1){
      del_ss_pag(sysPT, taskFrameID);
      del_ss_pag(sysPT, PDframeID);
      del_ss_pag(sysPT, childPTframe);
      free_frame(taskFrameID);
      free_frame(PDframeID);
      free_frame(childPTframe);
      for(int j = 0; j < i; j++) free_frame(frames[j]);
      return -ENOMEM;
    }

    // Map in child PT
    set_ss_pag(childPTU, i, frames[i], 1); 

    set_ss_pag(parentPTU, tmpmapping + i, frames[i], 1);

    copy_data((void *)(L_USER_START + i * PAGE_SIZE),
              (void *)(L_USER_START + ((tmpmapping + i) * PAGE_SIZE)), PAGE_SIZE);

    del_ss_pag(parentPTU, tmpmapping + i);
  }
  
  set_cr3(current()->dir_pages_baseAddr);
  
  int newpid = global_pid++;        //hay que poner robustez a las colisiones
  childunion->task.PID = newpid;



  /* 9. Forjar el contexto de retorno del hijo (fork devuelve 0) */
  unsigned long *ksp = (unsigned long *) KERNEL_ESP(childunion);
  ksp[-18] = 0;                               // EBP falso
  ksp[-17] = (unsigned long)ret_from_fork;    // retorno de set_esp
  ksp[-10] = 0;                               // EAX guardado = 0
  childunion->task.kernel_esp = (unsigned long *)&ksp[-18];

  /* 10. Estado y cola */
  childunion->task.state = ST_READY;
  childunion->task.quantum = current()->quantum;
  INIT_LIST_HEAD(&childunion->task.list);
  list_add_tail(&childunion->task.list, &readyqueue);

  return newpid;
  
}


void sys_exit(void)
{
    struct task_struct *t = current();
    page_table_entry *sysPT = get_PT(t);
    page_table_entry *dir = t->dir_pages_baseAddr;
    int dir_frame = ((unsigned long)dir) >> 12;
    int ptu_frame = dir[1].bits.pbase_addr;

    /* Si por lo que sea estuviera en una cola, sacarlo. */
    if (t->state != ST_RUN && t->list.next != 0)
        list_del(&t->list);

    /* 1. Datos + pila de usuario */
    if (ptu_frame != 0) {
        page_table_entry *ptu = (page_table_entry *)(ptu_frame << 12);

        for (int i = 0; i < NUM_PAG_DATA; i++) {
            int frame = get_frame(ptu, i);
            if (frame != 0) {
                del_ss_pag(ptu, i);
                del_ss_pag(sysPT, frame);
                free_frame(frame);
            }
        }

        /* 2. PT de usuario */
        del_ss_pag(dir, 1);
        del_ss_pag(sysPT, ptu_frame);
        free_frame(ptu_frame);
    }

    /* 3. Directorio de páginas */
    del_ss_pag(sysPT, dir_frame);
    free_frame(dir_frame);

    /* 4. El PCB no se puede liberar todavía: es nuestra pila.
          Lo dejamos apuntado para que otro lo libere. */
    t->state = ST_BLOCKED;
    list_add_tail(&t->list, &zombiequeue);

    /* 5. Elegir otro proceso y cambiar de contexto.
          Esto NO vuelve a este proceso. */
    sched_next_rr();

    /*Por si falla*/
    while (1);
}