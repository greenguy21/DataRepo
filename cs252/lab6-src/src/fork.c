#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(){
    while (1) {
		pid_t p = fork();
		if (p > 0) {
			printf("ForkProcess %d created.\n", p);
		}
	}
}
