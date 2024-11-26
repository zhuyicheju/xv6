#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main () {
    int p1[2], p2[2];
    if(pipe(p1) != 0 || pipe(p2) != 0){
        fprintf(2, "Pipe creation failure\n");
        exit(1);
    }
    int pid = fork();
    if(pid == 0){
        char* buf = malloc(1);
        read(p1[0], buf, 1);
        fprintf(1, "%d: received ping\n",getpid());
        close(p1[0]);
        write(p2[1], buf, 1);
        close(p2[1]);
    }else{
        char* buf = malloc(1);
        write(p1[1], "a", 1);
        close(p1[1]);
        read(p2[0], buf, 1);
        fprintf(1, "%d: received pong\n",getpid());
        close(p2[0]);
    }
    exit(0);
}