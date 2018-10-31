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
#define LOGFILESIZE 10000
#define SIZE 10

pid_t childpid = 0;
int i,m,k,x,k,s = 5,j,status,p,q,priority;
pid_t *child_pids;
key_t myshmKey, pcbKey,schedKey;
int shmId,pcbshmId,schedId, next_child_time, randomvalue, log_lines = 0, child_count = 0;
shared_mem *clock; 
scheduler_clock *scheduler;
pcb *process_control_blocks;
FILE *logfile;
char *file_name;
sem_t *mySemaphore;


void clearSharedMemory() {
fprintf(stderr, "Total Childs Forked : %d \n", child_count);
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
alarm(s);
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
if(file_name == NULL)
file_name = "default";
logfile = fopen(file_name, "w");

fprintf(stderr, "Opened Log File for writing Output::: %s \n", file_name);

if(logfile == NULL){
	perror("Error in opening file \n");
	exit(-1);
}

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
int filesize;
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
		//	child_count++;
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
		
		child_count++;
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
                 process_control_blocks[currentPCBBlock].processId = mypid;
                 process_control_blocks[currentPCBBlock].priority = priority;
		 process_control_blocks[currentPCBBlock].launch_time = (clock -> seconds * NANOSECOND) + clock -> nanoseconds;
		if(log_lines <10000)
		{
                log_lines++;
		fprintf(stderr, "OSS: Generating process with PID %d, with priority %d at time %d.%d with at location ####### %d priority ********************* %d \n", process_control_blocks[currentPCBBlock].processId, process_control_blocks[currentPCBBlock].priority, clock -> seconds, clock -> nanoseconds, currentPCBBlock, process_control_blocks[currentPCBBlock].priority);
		}
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
		 long int dispatch_end_secs, dispatch_end_nanosec;
		/*long int dispatch_start_sec = clock -> seconds;
		long int  dispatch_start_nano = clock -> nanoseconds;
		long int finalsec = (dispatch_start_sec - dispatch_end_secs);
		long int finalnanosec = (dispatch_start_nano - dispatch_end_nanosec);
		long int total_dispatch = finalsec * NANOSECOND + finalnanosec;*/
		if(log_lines <10000)
                {
                	log_lines++;
			fprintf(stderr, "OSS: Dispatching process with PID %d from queue 0 at %d:%ld \n", process_control_blocks[item].processId, clock -> seconds, clock -> nanoseconds);
			//fprintf(stderr, "OSS: Total time this dispatch took was %ld nanoseconds \n",total_dispatch);
		}
		sem_wait(mySemaphore);

		clock -> nanoseconds +=  process_control_blocks[item].used_burst;
		if(clock -> nanoseconds >= NANOSECOND)
	        {
        	clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
        	clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
        	}
		if(log_lines <10000)
                {
                	log_lines++;
			fprintf(stderr, "OSS: Receiving that process %d ran for %ld nanoseconds \n", process_control_blocks[item].processId, process_control_blocks[item].used_burst);
		}
		if(process_control_blocks[item].flag == 0)
		{
			if(process_control_blocks[item].used_burst != 10000000)
			{
			 if(log_lines <10000)
                		{
                		log_lines++;
				fprintf(stderr, "OSS: Not using its full quantum \n");
				}
			}
			enqueue(high_priority_queue, item);
			if(log_lines <10000)
			{                       
                        log_lines++;
			fprintf(stderr, "OSS: Putting process with PID %d into queue 0 \n",process_control_blocks[item].processId);
			}
		}
		else
			{
			fprintf(stderr, "OSS: Process with pid %d Completed \n ", process_control_blocks[item].processId);
			//fprintf(stderr, "OSS: Total_time Used %ld \n ", process_control_blocks[item].total_system_time);
			waitpid(process_control_blocks[item].processId, &status, 0);
			process_control_blocks[item].processId = 0;
			}
		dispatch_end_secs = clock -> seconds;
		dispatch_end_nanosec = clock -> nanoseconds;
		fprintf(stderr, "-------------------------------------------------------\n");
	}
	else if(isEmpty(low_priority_queue) > 0)
		{
			int item2 = dequeue(low_priority_queue);
                	scheduler -> processId = process_control_blocks[item2].processId;
			scheduler -> quantum = 20;
			long int dispatch_end_secs, dispatch_end_nanosec;
                	/*long int dispatch_start_sec = clock -> seconds;
                	long int  dispatch_start_nano = clock -> nanoseconds;
                	long int finalsec = (dispatch_start_sec - dispatch_end_secs);
                	long int finalnanosec = (dispatch_start_nano - dispatch_end_nanosec);
                	long int total_dispatch = finalsec * NANOSECOND + finalnanosec;*/
			fprintf(stderr, "OSS: Dispatching process with PID %d from queue 1 at %d:%ld \n", process_control_blocks[item2].processId, clock -> seconds, clock -> nanoseconds);
			fprintf(stderr, "OSS: Total time this dispatch took was %ld nanoseconds \n", item2);
			sem_wait(mySemaphore);

			clock -> nanoseconds +=  process_control_blocks[item2].used_burst;
			if(clock -> nanoseconds >= NANOSECOND)
        		{
       			clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
        		clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
        		}
	
			fprintf(stderr, "OSS: Receiving that process %d ran for %ld nanoseconds \n", process_control_blocks[item2].processId, process_control_blocks[item2].used_burst);
			if(process_control_blocks[item2].flag == 0)
			{
				if(process_control_blocks[item2].used_burst != 20000000)	
                		fprintf(stderr, "OSS: Not using its full quantum \n");
				enqueue(low_priority_queue, item2);
				fprintf(stderr, "OSS: Putting process with PID %d into queue 1 \n",process_control_blocks[item2].processId);
			}
			else
			{
        	        	fprintf(stderr, "OSS: Process with pid %d Completed \n ", process_control_blocks[item2].processId);
				//fprintf(stderr, "OSS: Total_time Used %ld \n ", process_control_blocks[item2].total_system_time);
				waitpid(process_control_blocks[item2].processId, &status, 0);
				process_control_blocks[item2].processId = 0;
			}
			dispatch_end_secs = clock -> seconds;
               	 	 dispatch_end_nanosec = clock -> nanoseconds;
			fprintf(stderr, "-------------------------------------------------------\n");
		}
	else 
		{
			clock -> seconds += 1;
			clock -> nanoseconds +=  randomNumberGenerator(0,1000);
                        if(clock -> nanoseconds >= NANOSECOND)
                        {
                        clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
                        clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
                        }
		}
}
clearSharedMemory();

return 0;
}
