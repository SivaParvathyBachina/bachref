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

#define NANOSECOND 1000000000

pid_t childpid = 0;
int i,m,k,x,k,s = 5,j,status,p;
pid_t *child_pids;
key_t myshmKey, pcbKey;
int shmId,pcbshmId, next_child_time, randomvalue;
shared_mem *clock; 
pcb *process_control_blocks;
FILE *logfile;
char *file_name;
pid_t *high_priority_queue;

void clearSharedMemory() {
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
shmdt((void *)process_control_blocks);
//fprintf(stderr,"Cloising File \n");
fprintf(stderr, "OSS started detaching OSS Clock Memory \n");
fprintf(stderr, "OSS started detaching shmMsg Memory \n");
shmctl(shmId, IPC_RMID, NULL);
shmctl(pcbshmId, IPC_RMID, NULL);
//sem_unlink(SEMNAME);
//fprintf(stderr, "Unlinked Semaphore \n");
fprintf(stderr, "OSS Cleared the Shared Memory \n");
}


void killExistingChildren(){
for(k=0; k<s; k++)
{
if(child_pids[k] != 0)
{
fprintf(stderr, "Killing child with Id %d \n", child_pids[k]);
kill(child_pids[k], SIGTERM);
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

if (argc < 2){ // check for valid number of command line arguments
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

signal(SIGALRM, myhandler);
alarm(2);
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

for(i = 0;i<18;i++){
	process_control_blocks[i].processId = 0;
	process_control_blocks[i].total_system_time = 0;
	process_control_blocks[i].cpu_time = 0;
	process_control_blocks[i].launch_time = 0;
	process_control_blocks[i].quantum = 0;
	process_control_blocks[i].priority = 0;
	process_control_blocks[i].remaining_time = 0;
	process_control_blocks[i].flag = 0;	
}
child_pids = (pid_t *)malloc(18 * sizeof(int));
high_priority_queue = (pid_t *)malloc(20 * sizeof(int));
pid_t mypid;
i = 1;
while(i < 4) {

//	if(((clock -> seconds * NANOSECOND) + clock -> nanoseconds) >= next_child_time)
	//{
	int choice = randomNumberGenerator(0,1);
	//if(choice == 0)
		child_pids[i] = fork();
                if(child_pids[i] == 0)
                {
		mypid = getpid();
		high_priority_queue[j] = child_pids[i];
                char argument2[20],argument3[50], argument4[4], argument5[10];
                char *s_val = "-s";
		char *pcbshmVal2 = "-j";
		char *pidVal = "-p";
		fprintf(stderr, "%d \n", mypid);
		char *arguments[] = {NULL,pidVal,argument2,s_val, argument3,pcbshmVal2, argument4, NULL};
                arguments[0]="./user";
		sprintf(arguments[2], "%d", i);
                sprintf(arguments[4], "%d", shmId);
                sprintf(arguments[6], "%d", pcbshmId);
                process_control_blocks[i].launch_time = (clock -> seconds*NANOSECOND + clock ->nanoseconds);
                process_control_blocks[i].processId = mypid;
		process_control_blocks[i].quantum = 5;
                execv("./user", arguments);
                fprintf(stderr, "Error in exec");
		}
	//next_child_time = ((clock -> seconds + ((rand() % 3)+1)) + clock -> nanoseconds)
//	}
	
	randomvalue = randomNumberGenerator(0, 1000);
	if(clock -> nanoseconds >= NANOSECOND) 
	{
	clock -> seconds += 1;
	clock -> nanoseconds = 0;
	}
	clock -> nanoseconds += randomvalue;
	clock -> seconds += 1;	
	i++;
}

while((waitpid(-1, &status, 0) > 0 )){};
clearSharedMemory();

return 0;
}
