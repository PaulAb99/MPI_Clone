
#include <stdio.h>
#include <unistd.h>
int main(){

    printf("I am process %d made by %d at %s\n", getpid(), getppid(), __TIME__);
}