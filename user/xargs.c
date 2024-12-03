#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/stat.h"

int main (int argc, char* argv[]){
    static char buf[512];
    char* p = buf;
    char* args[MAXARG];
    int st = 1,idx = argc-1;
    for(int i = 0; i < argc-1; i++){
        args[i] = argv[i+1];
    }
    while(read(0, p, 1) == 1){
        if(st == 1 && *p != ' ' && *p != '\n') st = 0, args[idx++] = p;
        if(*p == ' ') *p = 0, st = 1;
        if(*p == '\n') {
            *p = 0, st = 1;
            args[idx] = 0;
            idx = argc-1;
            if(fork() == 0){
                exec("echo", args);
                fprintf(2, "xargs: exec failed\n");
            }
            int status;
            wait(&status);
        }
        p ++;
    }
    if(st == 0){
        args[idx] = 0;
        idx = argc-1;
        if(fork() == 0){
            exec("echo", args);
            fprintf(2, "xargs: exec failed\n");
        }
        int status;
        wait(&status);    
    }
    exit(0);
}
