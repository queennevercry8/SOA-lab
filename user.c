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


static void test_fork_exit()
{
    int i, yo = getpid(), pid;
    char buf[64];

    /* --- bucle: crear 3 hijos --- */
    for (i = 0; i < 3; i++) {
        pid = fork();
        if (pid < 0) { perror(); exit(); }

        if (pid == 0) {                    /* --- HIJO --- */
            int n = 0, p = getpid();
            buf[n++] = 'H'; buf[n++] = 'i'; buf[n++] = 'j'; buf[n++] = 'o';
            buf[n++] = ' '; buf[n++] = 'P'; buf[n++] = 'I'; buf[n++] = 'D';
            buf[n++] = '=';
            if (p == 0) buf[n++] = '0';
            else { char t[12]; int k = 0;
                   while (p) { t[k++] = '0' + p % 10; p /= 10; }
                   while (k) buf[n++] = t[--k]; }
            buf[n++] = '\n';
            write(1, buf, n);
            exit();                       /* el hijo muere: no vuelve al for */
        }
    }

    /* --- solo el padre llega aquí --- */
    {
        int n = 0, p = yo;
        char t[12]; int k = 0;
        buf[n++] = 'P'; buf[n++] = 'a'; buf[n++] = 'd'; buf[n++] = 'r'; buf[n++] = 'e';
        buf[n++] = ' '; buf[n++] = 'v'; buf[n++] = 'i'; buf[n++] = 'v'; buf[n++] = 'o';
        buf[n++] = ' '; buf[n++] = 'P'; buf[n++] = 'I'; buf[n++] = 'D'; buf[n++] = '=';
        if (p == 0) buf[n++] = '0';
        else { while (p) { t[k++] = '0' + p % 10; p /= 10; }
               while (k) buf[n++] = t[--k]; }
        buf[n++] = '\n';
        write(1, buf, n);
    }
    exit();                               /* el padre muere al final */
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
    write(1, "\n", 1);

    test_fork_exit();

    while (1)
    {
    }
}
