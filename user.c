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

void test_cyclic_printing() {
    char *textos[] = {
        "Uno\n",
        "Dos\n",
        "Tres\n"
    };
    
    int num_textos = sizeof(textos) / sizeof(textos[0]);
    
    for (int i = 0; i < 33; i++) {
        int indice = i % num_textos;
        
        write(1, textos[indice], strlen(textos[indice]));
    }
}

int global_var = 100; // Variable global para testear copia en copia (COW lógico)

void test_basic_fork() {
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
        write(1, "Soy el padre. Mi PID es ", 24);
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

void test_nested_fork() {
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

//Test de estrés de memoria (crear y destruir en bucle para detectar fugas / leaks o quedarse sin frames)
void test_stress_memory() {
    int i;    
    for (i = 0; i < 5000; i++) {
        int pid = fork();
        if (pid < 0) {
            write(1, "¡ERROR: Agotamiento de memoria (Sin frames)! Fuga detectada.\n", 59);
            exit();
        }
        if (pid == 0) {
            write(1, "Soy proceso con PID = ", 22);
            int p = getpid();
            itoa(p, buff);
            write(1, buff, 5);
            write(1, "\n", 1);
            // El hijo muere instantáneamente liberando sus recursos
            exit();
        }
    }

    write(1, "Test de estres de memoria superado con exito (liberacion correcta)\n", 67);
    exit();
}

void test_nested_fork_loop() {
    char buff[20];
    int niveles = 3;

    write(1, "INICIO TEST BUCLE FORK ANIDADADO\n", 33);

    for (int i = 0; i < niveles; i++) {
        int pid = fork();
        if (pid < 0) {
            write(1, "Error en fork anidado\n", 22);
            perror();
            exit();
        }
        
        if (pid == 0) {
            // Cada hijo recién creado imprime su nivel y continúa el bucle para crear su propio hijo
            write(1, "Proceso en nivel ", 18);
            int p = getpid();
            itoa(p, buff);
            write(1, buff, 2);
            write(1, "\n", 1);
        } else {
            // El padre de esta iteración rompe el bucle o espera para no duplicar ramas descontroladamente
            break; 
        }
    }

    for(int d = 0; d < 100000; d++);

    write(1, "Proceso finalizando\n", 20);
    exit();
}

void recursive_test(int depth){
    if(depth == 0) return;
    for(int i = 0; i < 3; i++){
        int pid = fork();
        if(pid < 0){
            write(1, "Error en fork anidado\n", 22);
            perror();
            exit();
        }
        if(pid == 0){
            write(1, "Soy proceso con PID = ", 22);
            int p = getpid();
            itoa(p, buff);
            write(1, buff, 2);
            write(1, "\n", 1);
            itoa(depth, buff);
            write(1, buff, 2);
            write(1, "\n", 1);
            recursive_test(depth - 1);
            exit();
        }

    }
}

//Test de padre muriendo antes que los hijos
void test_parent_dies_first() {
    char buff[20];

    write(1, "INICIO TEST PADRE MUERE ANTES\n", 30);

    int pid = fork();
    if (pid < 0) {
        write(1, "Error en fork\n", 14);
        perror();
        exit();
    }

    if (pid == 0) {
        for (int d = 0; d < 150000; d++){}

        write(1, "Hijo sigue vivo despues de que el padre muriera. PID = ", 55);
        int p = getpid();
        itoa(p, buff);
        write(1, buff, 2);
        write(1, "\n", 1);
        
        write(1, "FIN TEST PADRE MUERE ANTES\n", 26);
        exit();
    } else {
        write(1, "Padre imprimiendo y muriendo inmediatamente...\n", 48);
        exit();
    }
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
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

  //test_cyclic_printing(); //test escriptura loop

  //JUEGOS DE PRUEBA->DESCOMENTAR EL DESEADO
  test_basic_fork();
  //test_nested_fork();
  //test_stress_memory();
  //test_nested_fork_loop();
  //recursive_test(3);
  //test_parent_dies_first();

  while(1) { }
}
