#include <libc.h>
#include <errno.h>
#include <stddef.h>

char buff[24];

int result, asm_result;

int pid;

int add(int par1, int par2)
{
    return par1 + par2;
}

int addAsm(int, int);
int test_enosys(void);
int test_sys_write_bad_buff(void);

int __attribute__((__section__(".text.main")))
main(void)
{
    // result = add(6, 47818);
    // result = add(51914, 51914);
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
    /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    // asm_result = addAsm(result, 51914);
    char *message = "Hello World from ZeOS!\n";
    write(1, message, strlen(message));

    for (int i = 0; i < 100000; i++)
        ;

    int ticks = gettime();

    char time_message[256];
    itoa(ticks, time_message);

    write(1, "Ticks since boot: ", strlen("Ticks since boot: "));
    write(1, time_message, strlen(time_message));

    int pid = fork();
    if (pid == 0)
    {
        write(1, "Soy el hijo\n", 12);
        write(1, "getpid hijo: ", 13);
        char b[8];
        itoa(getpid(), b);
        write(1, b, strlen(b));
        write(1, "\n", 1);
    }
    else
    {
        write(1, "Soy el padre\n", 13);
        write(1, "getpid padre: ", 14);
        char b[8];
        itoa(getpid(), b);
        write(1, b, strlen(b));
        write(1, "\n", 1);
        write(1, "fork devolvio: ", 15);
        itoa(pid, b);
        write(1, b, strlen(b));
        write(1, "\n", 1);
    }

    while (1)
    {
    }
}
