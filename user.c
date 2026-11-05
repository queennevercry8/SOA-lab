#include <libc.h>
#include <errno.h>
#include <stddef.h>

char buff[24];

int result, asm_result;

int pid;

int add(int par1,int par2){
	return par1+par2;
}

int addAsm(int,int);
int test_enosys(void);
int test_sys_write_bad_buff(void);


int __attribute__ ((__section__(".text.main")))
  main(void)
{
	//result = add(6, 47818);
	//result = add(51914, 51914);
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	//asm_result = addAsm(result, 51914);
  char *message = "Hello World from ZeOS!\n";
  write(1, message, strlen(message));


 write(1, "Starting testing 0o0\n", strlen("Starting testing 0o0\n"));

  // 1. Prueba EBADF (Bad file descriptor)
  write(99, message, strlen(message));
  if (errno == EBADF) {
      write(1, "EBADF capturado: ", 17);
      perror();
  }

  // 2. Prueba EFAULT (Puntero de parámetros nulo en %ebx)
  int res_fault_buff = test_sys_write_bad_buff();
  if (res_fault_buff < 0) {
      errno = -res_fault_buff;
      if (errno == EFAULT) {
          write(1, "EFAULT (buff puntero nulo) capturado: ", 38);
          perror();
      }
  }

  // 3. Prueba EFAULT (Búfer de datos nulo)
  write(1, NULL, 10);
  if (errno == EFAULT) {
      write(1, "EFAULT (buffer == NULL) capturado: ", 35);
      perror();
  }

  // 4. Prueba EINVAL (size < 0)
  write(1, message, -5);
  if (errno == EINVAL) {
      write(1, "EINVAL (size negativo) capturado: ", 34);
      perror();
  }

  // 5. Prueba EFAULT (Dirección de memoria no válida)
  write(1, (char *)0xFFFFFFFF, 10);
  if (errno == EFAULT) {
      write(1, "EFAULT (access_ok write fallo) capturado: ", 41);
      perror();
  }

  // 6. Prueba ENOSYS (Function not implemented)
  int res_enosys = test_enosys();
  if (res_enosys < 0) {
      errno = -res_enosys;
      if (errno == ENOSYS) {
          write(1, "ENOSYS capturado: ", 17);
          perror();
      }
  }

  write(1, "Finished testing :D\n", strlen("Finished testing :D\n"));

  for(int i = 0; i < 100000; i++);

  int ticks = gettime();

  char time_message[256];
  itoa(ticks, time_message);

  write(1, "Ticks since boot: ", strlen("Ticks since boot: "));
  write(1, time_message, strlen(time_message));

  while(1) { }
}
