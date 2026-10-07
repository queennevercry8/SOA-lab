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
int sys_write(int fd, char * buffer, int size)
{
  int ret = check_fd(fd, ESCRIPTURA);
  if (ret < 0) return ret;

  //In the future we need to check that it is a valid address 
  if(buffer == NULL) return -14; /*EFAULT*/  
  if (size < 0) return -22;    // -EINVAL (Invalid argument)
  if (size == 0) return 0;

  //existe mapeo a dirección física?
  if(access_ok(VERIFY_WRITE, buffer, size) == 0) return -14; /*EFAULT*/
  
  int bytes_to_copy = (size > 1024) ? 1024 : size;

  char kernel_buffer[1024];
  
  for(int i = 0; i < bytes_to_copy; i+=256){
    if(bytes_to_copy%256 != 0 && i + 256 > bytes_to_copy){
      copy_from_user(buffer + i, kernel_buffer + i, bytes_to_copy%256);
    } else copy_from_user(buffer + i, kernel_buffer + i, 256);
  }
  int written = sys_write_console(kernel_buffer, bytes_to_copy);
  return written;
}

int sys_gettime(){
  return zeos_ticks;
}
