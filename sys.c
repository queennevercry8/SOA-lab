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
  struct task_struct *parent = current();
  page_table_entry *sysPT = get_PT(parent);   // PT de sistema (entrada 0)

  /* 1. PCB del hijo: mapear ANTES de usarlo */
  int taskFrameID = alloc_frame();
  if (taskFrameID < 0) return -ENOMEM;
  set_ss_pag(sysPT, taskFrameID, taskFrameID, 0);

  /* 2. Directorio de páginas del hijo */
  int PDframeID = alloc_frame();
  if (PDframeID < 0) return -ENOMEM;
  set_ss_pag(sysPT, PDframeID, PDframeID, 0);

  /* 3. PT de usuario del hijo */
  int childPTframe = alloc_frame();
  if (childPTframe < 0) return -ENOMEM;
  set_ss_pag(sysPT, childPTframe, childPTframe, 0);

  union task_union *child = (union task_union *)(taskFrameID << 12);
  page_table_entry *childDir = (page_table_entry *)(PDframeID << 12);
  page_table_entry *childPTU = (page_table_entry *)(childPTframe << 12);
  page_table_entry *parentPTU =
      (page_table_entry *)(parent->dir_pages_baseAddr[1].bits.pbase_addr << 12);

  /* 4. Copiar task_union del padre (incluye la pila de kernel) */
  copy_data(parent, child, sizeof(union task_union));

  /* 5. Directorio: compartir PT de sistema + PT de usuario nueva */
  clear_page_table(childDir);
  childDir[0].entry = parent->dir_pages_baseAddr[0].entry;
  set_ss_pag(childDir, 1, childPTframe, 1);   // esto faltaba (PageDir[1])
  child->task.dir_pages_baseAddr = childDir;

  /* 6. Código compartido (índices 20..27 = PAG_LOG_INIT_CODE & 0x3FF) */
  clear_page_table(childPTU);                 // esto faltaba
  for (int i = NUM_PAG_DATA; i < NUM_PAG_DATA + NUM_PAG_CODE; i++) {
    set_ss_pag(childPTU, i, get_frame(parentPTU, i), 1);
    childPTU[i].bits.rw = 0;                  // código solo lectura
  }

  /* 7. Datos + pila: frames nuevos, copiados por la PT de sistema */
  for (int i = 0; i < NUM_PAG_DATA; i++) {
    int frame = alloc_frame();
    if (frame < 0) return -ENOMEM;
    set_ss_pag(sysPT, frame, frame, 0);       // visible desde el kernel
    set_ss_pag(childPTU, i, frame, 1);
    copy_data((void *)(L_USER_START + i * PAGE_SIZE),
              (void *)(frame << 12), PAGE_SIZE);
  }

  /* 8. PID */
  int newpid = global_pid++;
  child->task.PID = newpid;

  /* 9. Forjar el contexto de retorno del hijo (fork devuelve 0) */
  unsigned long *ksp = (unsigned long *) KERNEL_ESP(child);
  ksp[-18] = 0;                               // EBP falso
  ksp[-17] = (unsigned long)ret_from_fork;    // retorno de set_esp
  ksp[-10] = 0;                               // EAX guardado = 0
  child->task.kernel_esp = (unsigned long *)&ksp[-18];

  /* 10. Estado y cola */
  child->task.state = ST_READY;
  child->task.quantum = parent->quantum;
  INIT_LIST_HEAD(&child->task.list);
  list_add_tail(&child->task.list, &readyqueue);

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