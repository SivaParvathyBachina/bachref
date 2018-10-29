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

int q;
int getPCBBlockId(int processId)
{
        for(q = 0; q<18;q++)
        {
                if(process_control_blocks[q].processId == processId)
                return q;
        }
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
	//fprintf(stderr, "%s \n", semName);
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

int choice;
float value;

int q = 0;
int randomNumber;
int  mypid = getpid();
	while(1) 
	{
	if((scheduler -> processId) == mypid)
	{
		randomNumber = randomNumberGenerator(0,100);
		if(randomNumber <= 10)
        		choice = 0;
		else if(randomNumber <= 80)
        		choice = 1;
		else if(randomNumber <= 90)
        		choice = 2;
		else
        		choice = 3;

			if(choice == 0)
			{
				scheduler -> quantum = quantum_used * 1000;
			}
			else if(choice == 1)
			{
				float quant = (float) scheduler -> quantum;
				quantum_used =(float) (quant/1000);
				wait(quantum_used);
				int blockId = getPCBBlockId(scheduler -> processId);
				process_control_blocks[blockId].cpu_time += (quantum_used);
				process_control_blocks[blockId].used_burst = quantum_used * NANOSECOND;	
			}
			else if(choice == 2)
			{
				int r = randomNumberGenerator(0,5);
                                int s = randomNumberGenerator(0,1000);
				float t = (float)s/1000;
				quantum_used =(float)  r + t;
                                wait(quantum_used);
                                int blockId = getPCBBlockId(scheduler -> processId);
				process_control_blocks[blockId].cpu_time += quantum_used;
				process_control_blocks[blockId].used_burst = quantum_used * NANOSECOND;
			}
			else
			{
				int percent = randomNumberGenerator(1,99);
				float p = (float)percent / 100;
				float t = (float)scheduler -> quantum / 1000;
				quantum_used =(float) p * t;
				wait(quantum_used);
				int blockId = getPCBBlockId(scheduler -> processId);
				process_control_blocks[blockId].cpu_time += quantum_used;
				process_control_blocks[blockId].used_burst = quantum_used * NANOSECOND;
			}
	        quantum_used = 0;
		int pcb_id = getPCBBlockId(mypid);
		if(process_control_blocks[pcb_id].cpu_time >= 0.05)
		{
			int termination = randomNumberGenerator(0,100);
			if(termination <= 50)
			{
			process_control_blocks[pcb_id].flag = 1;
			sem_post(mySemaphore);
			break;
			}
		}	 	
		 scheduler -> processId = -1;
                 sem_post(mySemaphore);
	}
	}
return 0;
}
