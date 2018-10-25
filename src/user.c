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
int shmId,pcbshmId, x,n, processId;
pid_t childpid;

int randomNumberGenerator(int min, int max)
{
	return ((rand() % (max-min +1)) + min);
}

int main (int argc, char *argv[]) {

childpid = getpid();

while((x = getopt(argc,argv, "p:s:j:")) != -1)
switch(x)
{
case 'p': 
	processId = atoi(optarg);
	break;
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

fprintf(stderr, "Pid in User  %d \n", processId);

int j;
//for(j = 0;j<20;j++)
//{
//	if(process_control_blocks[j].processId == processId)
	fprintf(stderr, "%d Launch time \n", process_control_blocks[processId].launch_time);
	
//}

//int value = randomNumberGenerator(0,3);

//fprintf(stderr, "%d Launch time \n", process_control_blocks[processId].launch_time);

return 0;
}
