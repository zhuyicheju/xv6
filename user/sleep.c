#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(2, "Usage: sleep [time]...\n");
        exit(1);
    }

    int time = atoi(argv[1]);
    fprintf(1, " Sleep %ds\n", time);
    int ret = sleep(time);
    if(ret != 0){
        fprintf(1, "Sleep interrupt with %ds left\n", ret);
    }
    exit(0);
}