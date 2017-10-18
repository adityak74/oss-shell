#ifndef _SHM_HEADER_H
#define _SHM_HEADER_H

// #define SHM_KEY 19942017
#define MAX_BUF_SIZE 1024

typedef struct 
{
	long seconds;
	long nanoseconds;
	char shmMsg[MAX_BUF_SIZE];
} shared_oss_struct;

typedef struct 
{
	long sec;
	long nano;
	pid_t procID;
} shmMsg;

#endif