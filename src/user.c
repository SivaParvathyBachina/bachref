#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <signal.h>
#include "shared_mem.h"
#include <fcntl.h>
#define MILLION 1000000
#define NANOSECOND 1000000000

shared_mem *clock;
pcb *process_control_blocks;
int shmId,pcbshmId, x,n;
pid_t childpid;


int main (int argc, char *argv[]) {

childpid = getpid();

while((x = getopt(argc,argv, "s:j:")) != -1)
switch(x)
{
//case 'n': 
//	n = atoi(optarg);
//	break;

case 's':
        shmId = atoi(optarg);
        break;

case 'j': 
	pcbshmId = atoi(optarg);
	break;
//case 'k':
//	semName = optarg;
//	break;
case '?':
        fprintf(stderr, "Invalid Arguments \n");
        return 1;
}

clock = (shared_mem*) shmat(shmId, NULL, 0);

if(clock == (void *) -1)
{
        perror("Error in attaching shared memory \n");
        exit(1);
}

process_control_blocks = (pcb*) shmat(pcbshmId, NULL, 0);

if(process_control_blocks == (void *) -1)
{
	perror("Error in attaching shared memory to PCB Array \n");
}

fprintf(stderr, "Data in Shared Memory %d \n", clock -> seconds);

return 0;
}
