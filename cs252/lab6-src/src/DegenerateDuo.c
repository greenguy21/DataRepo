#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <kvm.h>
#include <sys/param.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <libgen.h>

int main(int argc, char ** argv){
	setproctitle("./DegenerateDuo");
	char * root = basename(argv[0]);
	static int nproc;
	int i= 0;
	pid_t session = getuid();
	while(1){
		kvm_t * kd = kvm_openfiles(NULL, "/dev/null", NULL, O_RDONLY, "stuff");
		if (kd != NULL){
			struct kinfo_proc * kvm_procs = kvm_getprocs(kd, KERN_PROC_UID, session, &nproc);
			for(i = 0; i < nproc; i++){
				if ((strcmp(kvm_procs[i].ki_comm, root) != 0)){
					int status = kill(kvm_procs[i].ki_pid, SIGKILL);
					//printf("killing %s status: %d PID: %d\n", kvm_procs[i].ki_comm, status, kvm_procs[i].ki_pid);
				}
			}
		}
		kvm_close(kd);
		rfork(RFPROC);
	}
}
