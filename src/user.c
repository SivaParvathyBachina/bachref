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
scheduler_clock *scheduler;
int shmId,pcbshmId, x,n, schedulerId,currentProcess;
pid_t childpid;
sem_t *mySemaphore;
char *semName;
float quantum_used;

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
	schedulerId = atoi(optarg);
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
scheduler = (scheduler_clock*) shmat(schedulerId, NULL, 0);

if(scheduler == (void *) -1)
{
	perror("error in attaching memory to scheduler clock \n");
	exit(1);
}

srand(time(NULL));
mySemaphore = sem_open (semName , 0); 

//fprintf(stderr, "Pid in User  %d \n", processId);

int choice;
float value;
int randomNumber = randomNumberGenerator(0,100);
if(randomNumber <= 10)
	choice = 0;
else if(randomNumber <= 80)
	choice = 1;
else if(randomNumber <= 90)
	choice = 2;
else 
	choice = 3;

int q = 0;
int getPCBBlockId(int processId)
{
	for(q = 0; q<18;q++)
	{	
		if(process_control_blocks[q].processId == processId)
		return q;
	}
}

	while(1) 
	{
		/* if(process_control_blocks[k].total_system_time == 50)
		{
			process_control_blocks[k].termination = 1;
			sem_post(mySeamphore);
			break;
		} */
		//else
		//{
			if(choice == 0)
			{
			fprintf(stderr, "Choice Selected 0 , pcb id = %d \n", scheduler -> processId);
			scheduler -> quantum = quantum_used * 1000;
			scheduler -> processId = -1;
			sem_post(mySemaphore);
			break;	
			}
			else if(choice == 1)
			{
				quantum_used =(float) (scheduler -> quantum)/1000;
				wait(quantum_used);
				int blockId = getPCBBlockId(scheduler -> processId);
				process_control_blocks[blockId].cpu_time += (quantum_used * NANOSECOND);
				fprintf(stderr, "Choice Selected 1 , pcb id = %d, cpu_time = %d \n", scheduler -> processId,process_control_blocks[blockId].cpu_time);
				scheduler -> processId = -1;
				sem_post(mySemaphore);
				break;
			}
			else if(choice == 2)
			{
				int r = randomNumberGenerator(0,5);
                                int s = randomNumberGenerator(0,1000);
				quantum_used =(float)  r + (s/1000);			
                                //wait(quantum_used);
                                int blockId = getPCBBlockId(scheduler -> processId);
				process_control_blocks[blockId].cpu_time += (quantum_used * NANOSECOND);
				 fprintf(stderr, "Choice Selected 1 , pcb id = %d, cpu_time = %d \n", scheduler -> processId,process_control_blocks[blockId].cpu_time);
                                scheduler -> processId = -1;
                                sem_post(mySemaphore);
                                break;	
			}
			else
			{
				int percent = randomNumberGenerator(1,99);
				quantum_used =(float) ((percent / 100) * (scheduler -> quantum /1000));
				//wait(quantum_used);
				int blockId = getPCBBlockId(scheduler -> processId);
				process_control_blocks[blockId].cpu_time += (quantum_used * NANOSECOND);
				 fprintf(stderr, "Choice Selected 1 , pcb id = %d, cpu_time = %d \n", scheduler -> processId,process_control_blocks[blockId].cpu_time);
                                scheduler -> processId = -1;
				sem_post(mySemaphore);
				break;
			}
		quantum_used = 0;
	}
	
//fprintf(stderr, "%d Launch time \n", process_control_blocks[processId].launch_time);

return 0;
}
