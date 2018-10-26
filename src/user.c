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
#include <semaphore.h>

shared_mem *clock;
pcb *process_control_blocks;
int shmId,pcbshmId, x,n, processId,currentProcess;
pid_t childpid;
sem_t *mySemaphore;
char *semName;

int randomNumberGenerator(int min, int max)
{
	return ((rand() % (max-min +1)) + min);
}

int main (int argc, char *argv[]) {

childpid = getpid();

while((x = getopt(argc,argv, "p:k:s:j:")) != -1)
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
case 'k':
	semName = optarg;
	break;
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

srand(time(NULL));
mySemaphore = sem_open (semName , 0); 

fprintf(stderr, "Pid in User  %d \n", processId);

int j;
//for(j = 0;j<20;j++)
//{
//	if(process_control_blocks[j].processId == processId)
	fprintf(stderr, "%d Clock Time \n", (clock -> seconds * NANOSECOND + clock -> nanoseconds));
	
//}
int k;
for (k=0; k<20; k++)
{
	if(process_control_blocks[k].processId == processId)
	currentProcess = k;
	break;
}
int choice = randomNumberGenerator(0,3);
	/*while(1) 
	{
		if(process_control_blocks[k].total_system_time == 50)
		{
			process_control_blocks[k].termination = 1;
			sem_post(mySeamphore);
			break;
		}
		else
		{
			if(choice == 0)
			{
			sem_post(mySemaphore);
			break;	
			}
			else if(choice == 1)
			{
			
			}
			else if(choice == 2)
			{
				int r = randomNumberGenerator(0,5);
                                int s = randomNumberGenerator(0,1000);
				//if(s < 10)
					//s = s/10;
                                wait(r.s);
                                sem_post(mySemaphore);
                                break;	
			}
			else
			{
				int percent = randomNumberGenerator(1,99);
				int quan_to_be_used = (percent/100) * quantum;	
				sem_post(mySemaphore);
				break;
			}
		}	
	} */

//fprintf(stderr, "%d Launch time \n", process_control_blocks[processId].launch_time);

return 0;
}
