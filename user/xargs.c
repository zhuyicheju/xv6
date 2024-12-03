#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/stat.h"

int main (int argc, char* argv[]){
    static char buf[512];
    char* p = buf;
    char* args[MAXARG];
    int step = 0;
    //st = 1 is waiting for a new word
    //st = 0 is finished a word

    if(strcmp(argv[1],"-n") == 0) {
        step = atoi(argv[2]);
        if(step == 0){
            fprintf(2, "-n: step must >= 0\n");
            exit(1);
        }
    }

    int extra = (step == 0) ? 0 : 2;
    int st = 1,idx = argc-extra-1;
    //printf("%d\n",step);
    for(int i = 0; i < argc-extra-1; i++){
        args[i] = argv[i + extra + 1];
        //xargs -n 1 echo 1
    }

    int now_step = 0;
    while(read(0, p, 1) == 1){
        if(st == 1 && *p != ' ' && *p != '\n') st = 0, args[idx++] = p, now_step++;
        else if(*p == '\n' || (step == now_step && *p == ' ')) {
            *p = 0, st = 1;
            now_step = 0;
            args[idx] = 0;
            idx = argc-extra-1;
            if(fork() == 0){
                if(argc == 1) exec("echo", args);
                else exec(argv[1 + extra], args);
                fprintf(2, "xargs: exec [%s] failed\n", argv[1]);
            }
            int status;
            wait(&status);
        }
        else if(*p == ' ') *p = 0, st = 1;
        p ++;
    }
    if(st == 0){
        args[idx] = 0;
        idx = argc-1;
        if(fork() == 0){
            if(argc == 1) exec("echo", args);
            else exec(argv[1 + extra], args);
            fprintf(2, "xargs: exec [%s] failed\n", argv[1]);
        }
        int status;
        wait(&status);
    }
    exit(0);
}
