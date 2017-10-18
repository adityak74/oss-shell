#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "restart.h"
#define BUFSIZE 1024
#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)

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

pid_t r_wait(int *stat_loc) {
  pid_t retval;
  while (((retval = wait(stat_loc)) == -1) && (errno == EINTR)) ;
  return retval;
}

int main  (int argc, char *argv[]) {
   char buffer[BUFSIZE];
   char *c;
   pid_t childpid = 0;
   int delay;
   volatile int dummy = 0;
   int i, n;
   sem_t *semlockp;

   if (argc != 4){       /* check for valid number of command-line arguments */
      fprintf (stderr, "Usage: %s processes delay semaphorename\n", argv[0]);
      return 1;
   }
   n = atoi(argv[1]);
   delay = atoi(argv[2]);
   for (i = 1; i < n; i++)
      if (childpid = fork())
         break;
   snprintf(buffer, BUFSIZE,
      "i:%d  process ID:%ld  parent ID:%ld  child ID:%ld\n",
       i, (long)getpid(), (long)getppid(), (long)childpid);
   c = buffer;
   if (getnamed(argv[3], &semlockp, 1) == -1) {
      perror("Failed to create named semaphore");
      return 1;
   }
   while (sem_wait(semlockp) == -1)                         /* entry section */ 
       if (errno != EINTR) { 
          perror("Failed to lock semlock");
          return 1;
       }
   while (*c != '\0') {                                  /* critical section */
      fputc(*c, stderr);
      c++;
      for (i = 0; i < delay; i++)
         dummy++;
   }
   if (sem_post(semlockp) == -1) {                           /* exit section */
      perror("Failed to unlock semlock");
      return 1; 
   }
   if (r_wait(NULL) == -1)                              /* remainder section */
      return 1;
   return 0;
}