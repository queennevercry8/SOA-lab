#ifndef DEVICES_H__
#define  DEVICES_H__

#define N 8

extern struct list_head blocked;

//elegimos sobreescribir, no eliminar lo leído
//que sea bloqueante cuando el buffer está lleno
struct buffer_circular{

  char buffer[N];
  int idx_lec;
  int idx_esc;
  int n_char; //contador de elementos ocupados
  //sumamos cuando escribimos y restamos cuando leemos

};

int sys_write_console(char *buffer,int size);
#endif /* DEVICES_H__*/
