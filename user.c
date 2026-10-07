#include <libc.h>

char buff[24];

int result, asm_result;

int pid;

int add(int par1,int par2){
	return par1+par2;
}

int addAsm(int,int);

int __attribute__ ((__section__(".text.main")))
  main(void)
{
	//result = add(6, 47818);
	//result = add(51914, 51914);
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	//asm_result = addAsm(result, 51914);
  perror();
  char *message = "Hello World from ZeOS!\n";
  write(1, message, 30);

  for(int i = 0; i < 100000; i++);

  int ticks = gettime();

  char time_message[256];
  itoa(ticks, time_message);

  write(1, "Ticks since boot: ", strlen("Ticks since boot: "));
  write(1, time_message, strlen(time_message));

  while(1) { }
}
