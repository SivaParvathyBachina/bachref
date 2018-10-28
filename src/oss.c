#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include "shared_mem.h"
#define SHMSIZE 100
#define SEMNAME "sembach19181"
#include <semaphore.h>
#include "priority_queue.h"
#define NANOSECOND 1000000000
#define SIZE 10

pid_t childpid = 0;
int i,m,k,x,k,s = 5,j,status,p,q,priority;
pid_t *child_pids;
key_t myshmKey, pcbKey,schedKey;
int shmId,pcbshmId,schedId, next_child_time, randomvalue;
shared_mem *clock; 
scheduler_clock *scheduler;
pcb *process_control_blocks;
FILE *logfile;
char *file_name;
sem_t *mySemaphore;


void clearSharedMemory() {
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
shmdt((void *)process_control_blocks);
shmdt((void *)scheduler);
//fprintf(stderr,"Cloising File \n");
fprintf(stderr, "OSS started detaching OSS Clock Memory \n");
fprintf(stderr, "OSS started detaching shmMsg Memory \n");
shmctl(shmId, IPC_RMID, NULL);
shmctl(pcbshmId, IPC_RMID, NULL);
shmctl(schedId, IPC_RMID, NULL);
sem_unlink(SEMNAME);
fprintf(stderr, "Unlinked Semaphore \n");
fprintf(stderr, "OSS Cleared the Shared Memory \n");
}


void killExistingChildren(){
for(k=0; k<18; k++)
{
fprintf(stderr, "cpu : %f, pid : %d, flag :%d \n", process_control_blocks[k].cpu_time, process_control_blocks[k].processId, process_control_blocks[k].flag);
if(process_control_blocks[k].processId != 0)
{
//fprintf(stderr, "cpu : %d \n", process_control_blocks[k].cpu_time);
//fprintf(stderr, "Killing child with Id %d \n", process_control_blocks[k].processId);
kill(process_control_blocks[k].processId, SIGTERM);
}
}
}

void myhandler(int s) {
if(s == SIGALRM)
{
fprintf(stderr, "Master Time Done\n");
killExistingChildren();
clearSharedMemory();
}

if(s == SIGINT)
{
fprintf(stderr, "Caught Ctrl + C Signal \n");
fprintf(stderr, "Killing the Existing Children in the system \n");
killExistingChildren();
clearSharedMemory();
}
exit(1);
}

int randomNumberGenerator(int min, int max)
{
	return ((rand() % (max-min +1)) + min);
}


int main (int argc, char *argv[]) {

if (argc < 2){
fprintf(stderr, "Invalid number of arguments. Please give it in the following format");
fprintf(stderr, "Usage: %s  -n processess -h [help] -p [error message]", argv[0]);
return 1;
}
while((x = getopt(argc,argv, "hs:l:")) != -1)
switch(x)
{
case 'h':
        fprintf(stderr, "Usage: %s -s processCount -l logfile_name -t  -h [help]\n", argv[0]);
        return 1;
case 's':
	s = atoi(optarg);
	break;
case 'l':
	file_name = optarg;
	break;
//case 't': 
//	programRunTime = atoi(optarg); 
	break;
case '?':
        fprintf(stderr, "Please give '-h' for help to see valid arguments \n");
        return 1;
}


Queue* high_priority_queue = create_queue(20);
Queue* low_priority_queue = create_queue(20);

signal(SIGALRM, myhandler);
alarm(15);
signal(SIGINT, myhandler);

myshmKey = ftok(".", 'c');
shmId = shmget(myshmKey, sizeof(shared_mem), IPC_CREAT | 0666);

if(shmId <0 )
{
	fprintf(stderr, "Error in shmget \n");
	exit(1);
}

clock = (shared_mem*) shmat(shmId, NULL, 0);

if(clock == (void *) -1)
{
	perror("Error in attaching shared memory --- Master \n");
	exit(1);
}

fprintf(stderr, "Allocated Shared Memory For OSS Clock \n");

schedKey = ftok(".bac", 's');
schedId = shmget(schedKey, sizeof(scheduler_clock), IPC_CREAT | 0666);

if(schedId < 0)
{
	fprintf(stderr, "Error in scheduler Clock \n");
	exit(1);
}

scheduler = (scheduler_clock*) shmat(schedId, NULL, 0);

if(scheduler == (void *) -1)
{
	perror("Error in attaching csheduler \n");
	exit(1);
}

int pcbshmSize = sizeof(process_control_blocks) * 18;
pcbKey = ftok(".", 'x');
pcbshmId = shmget(pcbKey, pcbshmSize, IPC_CREAT | 0666);
if(pcbshmId < 0)
{
	fprintf(stderr, "Error in Shmget for PCB's \n");
	exit(1);
}

process_control_blocks = (pcb*) shmat(pcbshmId, NULL, 0);
if(process_control_blocks == (void *) -1)
{
	perror("Error in attaching Memory to process_control_blocks \n");
	exit(1);
}

mySemaphore = sem_open(SEMNAME, O_CREAT, 0666,0);

fprintf(stderr, "Created Semaphore with Name %s \n", SEMNAME);

clock -> seconds = 0;
clock -> nanoseconds = 0;
scheduler -> processId = 0;
scheduler -> quantum = 0;

q = 0;

srand(time(NULL));
for(i = 0;i<18;i++){
	process_control_blocks[i].processId = 0;
	process_control_blocks[i].total_system_time = 0;
	process_control_blocks[i].cpu_time = 0.0;
	process_control_blocks[i].launch_time = 0;
	process_control_blocks[i].priority = 0;
	process_control_blocks[i].used_burst = 0.0;
	process_control_blocks[i].flag = 0;	
}
child_pids = (pid_t *)malloc(18 * sizeof(int));

int currentPCBBlock = -1;
pid_t mypid;
while(1)
{
	int k;
	currentPCBBlock = -1;
	for(k = 0; k<18; k++)
	{
		if(process_control_blocks[k].processId == 0)
		{
		currentPCBBlock = k;
		break;
		}
	}
	
	/*if(currentPCBBlock == -1)
	fprintf(stderr, "PCB array is Full. Can't create new process \n");
	*/
	if((currentPCBBlock >= 0) && (clock -> seconds >= next_child_time))
	{
		if((mypid = fork()) ==0)
		{
			char argument2[20],argument3[50], argument4[4], argument5[10];
                	char *s_val = "-s";
			char *pcbshmVal2 = "-j";
			char *semVal = "-k";
			char *sched_val = "-p";
			char *arguments[] = {NULL,sched_val,argument2,s_val, argument3,pcbshmVal2, argument4,semVal, argument5, NULL};	
			arguments[0]="./user";
			sprintf(arguments[2], "%d", schedId);
			sprintf(arguments[4], "%d", shmId);
               		sprintf(arguments[6], "%d", pcbshmId);
			sprintf(arguments[8], "%s", SEMNAME);
			execv("./user", arguments);
                	fprintf(stderr, "Error in exec");
			exit(0); 
		}

		priority = randomNumberGenerator(0,100);
	        if(priority <=80)
                	 {
                		priority = 1;
                		enqueue(low_priority_queue, currentPCBBlock);
               		 }
       		 else
               		 {
               		 priority = 0;
               		 enqueue(high_priority_queue, currentPCBBlock);
               		 }

		 next_child_time += randomNumberGenerator(0,2);
		 child_pids[i] = mypid;
                 process_control_blocks[currentPCBBlock].processId = mypid;
                 process_control_blocks[currentPCBBlock].priority = priority;
		 process_control_blocks[currentPCBBlock].launch_time = (clock -> seconds * NANOSECOND) + clock -> nanoseconds;
        //        fprintf(stderr, "Forking Child in PCB Block %d, with ID %d, with Priority %d \n", currentPCBBlock, process_control_blocks[currentPCBBlock].processId,  process_control_blocks[currentPCBBlock].priority );
	}

	randomvalue = randomNumberGenerator(0, 1000);
	clock -> nanoseconds += randomvalue;
	if(clock -> nanoseconds >= NANOSECOND) 
	{
	clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
	clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
	}
	
	if(isEmpty(high_priority_queue) > 0)
	{
		int item = dequeue(high_priority_queue);
		scheduler -> processId = process_control_blocks[item].processId;
		scheduler -> quantum = 10;
		sem_wait(mySemaphore);
		clock -> nanoseconds +=  process_control_blocks[item].used_burst;
		if(process_control_blocks[item].flag == 0)
		enqueue(high_priority_queue, item);
	}
	else
	{
		if(isEmpty(low_priority_queue) > 0)
		{
			int item2 = dequeue(low_priority_queue);
                	scheduler -> processId = process_control_blocks[item2].processId;
			scheduler -> quantum = 20;
			sem_wait(mySemaphore);
			clock -> nanoseconds +=  process_control_blocks[item2].used_burst;
			if(process_control_blocks[item2].flag == 0)
			enqueue(low_priority_queue, item2);
		}
	}
}
//while((waitpid(-1, &status, 0) > 0 )){};
clearSharedMemory();

return 0;
}
