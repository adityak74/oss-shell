#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>

#include "shm_header.h"

#define PERM 0666
#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)

pid_t myPid;
shared_oss_struct *shpinfo;
int processNumber = 0;
const int QUIT_TIMEOUT = 10;
volatile sig_atomic_t sigNotReceived = 1;

// wait for semaphore
pid_t r_wait(int *stat_loc) {
  pid_t retval;
  while (((retval = wait(stat_loc)) == -1) && (errno == EINTR)) ;
  return retval;
}

// get the semaphore
int getnamed(char *name, sem_t **sem, int val) {
   while (((*sem = sem_open(name, FLAGS , PERMS, val)) == SEM_FAILED) &&
           (errno == EINTR)) ;
   if (*sem != SEM_FAILED)
       return 0;
   if (errno != EEXIST)
      return -1;
   while (((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR)) ;
   if (*sem != SEM_FAILED)
       return 0;
   return -1;
}

void intHandler(int);
void zombieKiller(int);

int main(int argc, char const *argv[])
{
	sembuf semsignal[1];
	sembuf semwait[1];
	int shmid = 0;
	long start_seconds, start_nanoseconds;
	long current_seconds, current_nanoseconds;
	myPid = getpid();
	char *short_options = "i:s:k:";
	int processNumber, maxproc;
	char c;

	//get options from parent process
	opterr = 0;
	while((c = getopt(argc, argv, short_options)) != -1)
		switch (c) {
			case 'i':
				processNumber = atoi(optarg);
				break;
			case 's':
				maxproc = atoi(optarg);
				break;
			case 'k':
				shmid = atoi(optarg);
				break;
			case '?':
				fprintf(stderr, "    Arguments were not passed correctly to slave %d. Terminating.", myPid);
				exit(-1);
		}

 	srand(time(NULL) + processNumber);

 	//Trying to attach to shared memory
	if((shpinfo = (shared_oss_struct *)shmat(shmid, NULL, 0)) == (void *) -1) {
		perror("    Slave could not attach shared mem");
		exit(1);
	}

	//Ignore SIGINT so that it can be handled below
	signal(SIGINT, intHandler);

	//Set the sigquitHandler for the SIGQUIT signal
	signal(SIGQUIT, intHandler);

	//Set the alarm handler
	signal(SIGALRM, zombieKiller);

	//Set the default alarm time
	alarm(QUIT_TIMEOUT);

	int i=0, j;

	long long duration = 1 + rand() % 1000000;
	printf("    Slave %d got duration %llu\n", processNumber, duration);

	start_seconds = shpinfo -> seconds;
	start_nanoseconds = shpinfo -> nanoseconds;
  	current_seconds = shpinfo -> seconds - start_seconds;
  	current_nanoseconds = shpinfo -> nanoseconds - start_nanoseconds;





  	printf("    Slave %d exiting\n", processNumber);
	kill(myPid, SIGTERM);
	sleep(1);
	kill(myPid, SIGKILL);
	printf("    Slave error\n");

	return 0;
}

//This handles SIGQUIT being sent from parent process
//It sets the volatile int to 0 so that it will not enter in the CS.
void intHandler(int sig) {
  printf("    Slave %d has received signal %s (%d)\n", processNumber, strsignal(sig), sig);
  sigNotReceived = 0;

  if(shmdt(shpinfo) == -1) {
    perror("    Slave could not detach shared memory");
  }

  kill(myPid, SIGKILL);

  //The slaves have at most 5 more seconds to exit gracefully or they will be SIGTERM'd
  alarm(5);
}

//function to kill itself if the alarm goes off,
//signaling that the parent could not kill it off
void zombieKiller(int sig) {
  printf("    Slave %d is killing itself due to slave timeout override%s\n", processNumber);
  kill(myPid, SIGTERM);
  sleep(1);
  kill(myPid, SIGKILL);
}