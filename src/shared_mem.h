#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

typedef struct
{
	long seconds;
	long nanoseconds;
}shared_mem;

typedef struct
{
	pid_t processId;
	long total_system_time;
	long cpu_time;
	long launch_time;
	long quantum;
	long remaining_time;
	int priority;
	int flag;
}pcb;
#endif
