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
  
}