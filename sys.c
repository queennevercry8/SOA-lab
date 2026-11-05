/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

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
  //1. Allocate frame for task struct
  int taskFrameID = alloc_frame();
  if(taskFrameID == -1){
    errno = ENOMEM;
    return -1;
  }
  union task_union *childunion = (union task_union *) (taskFrameID << 12);
  //struct task_struct  *childPCB = &(childunion->task); 
  
  //inherit system data by copying the parent's task_union
  copy_data(current(), childunion, sizeof(union task_union));

  //2. Allocate frame for page directory
  int PDframeID = alloc_frame();
  if(PDframeID == -1 ){
    errno = ENOMEM;
    return -1;
  }
  page_table_entry *PageDir = (page_table_entry *) (PDframeID << 12);
  clear_page_table(PageDir);

  //2.1.Shared System PT with the child + assign the child's PCB his own Page Directory
  
  /* ALTERNATIVA DIAPOSITIVAS
    int childPTSframe = alloc_frame();
    if (childPTSframe == -1){
      errno = ENOMEM;
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
  
  childunion->task.dir_pages_baseAddr = PageDir;

  //2.2. Inherit user data
  int childPTframe = alloc_frame();
  if(childPTframe == -1){
    errno = ENOMEM;
    return -1;
  }
  page_table_entry *childPTU = (page_table_entry *)(childPTframe << 12);
  
  page_table_entry *parentPTU = current()->dir_pages_baseAddr[1];
  //2.2.1.Map user code
  for(int i = NUM_PAG_DATA; i < NUM_PAG_DATA + NUM_PAG_CODE; i++){
    set_ss_pag(childPTU, i, get_frame(parentPTU, i), 1 );
  }

  //2.2.2. Map data + stack in new frames
  //TO FINISH I AM TIRED PLS END THIS SUFFERING

  int tmpmapping = NUM_PAG_CODE + NUM_PAG_DATA;

  for(int i = 0; i<NUM_PAG_DATA; i++){

    int frame = alloc_frame(); //reservar frame físico
    if(frame == -1){
      errno = NOMEM;
      return -1;
    }

    set_ss_pag(childPTU, i, frame, 1); //mapear en la tabla del hijo el frame físico

    set_ss_pag(parentPTU, tmpmapping, frame, 1 );

    char *src_addr = (char *) ((0x400 + i) << 12);
    char *dest_addr = (char *) ((0x400 + tmpmapping) << 12);
    copy_data(src_addr, dest_addr, 4096);
    del_ss_pag(parentPTU, tmpmapping);
  }

  set_cr3(current()->dir_pages_baseAddr);
  
  int newpid = global_pid++;        //hay que poner robustez a las colisiones
  childunion->task.PID = newpid;

  
  
}