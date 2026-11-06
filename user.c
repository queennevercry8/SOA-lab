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

int global_var = 100; // Variable global para testear copia en copia (COW lógico)

// 1. Test básico de jerarquía y padre que hace exit antes (o después)
static void test_basic_fork() {
    char buff[20];

    int pid = fork();
    if (pid < 0) {
        write(1, "Error en fork\n", 14);
        perror();
        exit();
    }
    if (pid == 0) {
        // HIJO
        write(1, "Soy el hijo. Mi PID es ", 23);
        int p = getpid();
        itoa(p, buff);
        write(1, buff, 2);
        write(1, "\n", 1);
        global_var = 200; // Modificamos variable global
        if (global_var == 200) {
            write(1, "Global modificada correctamente en hijo\n", 40);
        }
        exit();
    } else {
        // PADRE
        write(1, "Soy el padre. Mi PID es ", 23);
        int p = getpid();
        itoa(p, buff);
        write(1, buff, 2);
        write(1, "\n", 1);
        // Comprobamos que la variable global del padre no se ha visto alterada por el hijo
        if (global_var == 100) {
            write(1, "Aislamiento de memoria global correcto\n", 39);
        }
        exit(); // El padre muere (a veces antes de que el hijo termine si hay concurrencia)
    }
}

// 2. Test de hijos haciendo fork (árbol / fork en cascada)
static void test_nested_fork() {
    char buff[20];

    int pid = fork();
    if (pid < 0) {
        write(1, "Error en fork\n", 14);
        perror();
        exit();
    }

    if (pid == 0) {
        // Hijo 1 hace a su vez otro fork (crea nieto)
        write(1, "Soy el hijo. Mi PID es ", 23);
        int p = getpid();
        itoa(p, buff);
        write(1, buff, 2);
        write(1, "\n", 1);

        int pid2 = fork();
        if (pid2 == 0) {
            write(1, "Hola soy el NIETO. Mi PID es ", 30);
            int p = getpid();
            itoa(p, buff);
            write(1, buff, 2);
            write(1, "\n", 1);
            exit();
        }else{
            write(1, "Soy el padre del nieto. Mi PID es ", 34);
            int p = getpid();
            itoa(p, buff);
            write(1, buff, 2);
            write(1, "\n", 1);
            exit();
        }
        
    }

    write(1, "Soy el padre. Mi PID es ", 23);
    int p = getpid();
    itoa(p, buff);
    write(1, buff, 2);
    write(1, "\n", 1);
    for(int i = 0; i < 10000; i++)
    exit();
}

// 3. Test de estrés de memoria (crear y destruir en bucle para detectar fugas / leaks o quedarse sin frames)
static void test_stress_memory() {
    int i;
    char msg[32];
    
    for (i = 0; i < 2000; i++) {
        int pid = fork();
        if (pid < 0) {
            write(1, "¡ERROR: Agotamiento de memoria (Sin frames)! Fuga detectada.\n", 59);
            exit();
        }
        if (pid == 0) {
            write(1, "Soy proceso con PID = ", 22);
            int p = getpid();
            itoa(p, buff);
            write(1, buff, 4);
            write(1, "\n", 1);
            // El hijo muere instantáneamente liberando sus recursos
            exit();
        }
    }

    write(1, "Test de estres de memoria superado con exito (liberacion correcta)\n", 67);
    exit();
}


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
  write(1, "\n", 1);

  //JUEGOS DE PRUEBA->DESCOMENTAR EL DESEADO
  //test_basic_fork();
  //test_nested_fork();
  test_stress_memory();

  while(1) { }
}
