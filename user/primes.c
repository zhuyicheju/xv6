#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N 5
int p[2*N];
int primes[N];

void solve(int step){
    if(step >= N) return;
    if(fork() == 0){
        int prime, buf;
        read(p[step * 2], &prime, sizeof(int)); // the read point of the first pipe
        primes[step] = prime;
        fprintf(1, "Primes: %d\n", prime);
        solve(step + 1);
        while(read(p[step * 2], &buf, sizeof(int)) != 0){
            if(buf % prime != 0 && step * 2 + 3 < 2*N)
                write(p[step * 2 + 3], &buf, sizeof(int)); // the write point of the second point
        }
        close(p[step * 2 + 3]);
        close(p[step * 2]);
    }
}

int main () {
    for(int i = 0;i < N;i ++){
        pipe(p + i * 2);
    }
    for(int i = 2; i <= 113;i ++){
        write(p[1], &i, sizeof(int));
    }
    solve(0);
    exit(0);
}