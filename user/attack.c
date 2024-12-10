#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
  // your code here.  you should write the secret to fd 2 using write
  // (e.g., write(2, secret, 8)
  char* s = malloc(PGSIZE*128);
  char* secret = 0;
  for(int i = 0;i < PGSIZE*32-3; i++){
    if(*(s+i) == 'i' && *(s+i+1) == 's' && *(s+i+2) == ':'){
      secret = s + i + 4;
      write(2, secret, 8);
    }
  }
  exit(1);
}
