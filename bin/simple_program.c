#include <stdio.h>
#include <unistd.h>

int main(){
    printf("I am program %d made by %d\n",getpid(),getppid());
    return 0;
}