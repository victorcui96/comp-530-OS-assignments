 /*******************************
  * AUTHOR: Victor Cui
  *
  * PROBLEM DESCRIPTION: Implement HW1 using a bounded buffer ADT, memory-mapped files, and POSIX semaphores
  *
  * SOLUTION APPROACH: Use processes to pipeline work in workflow, & use pipes as a buffer that processes read / write data from
  * 
  *
  * UNC Honor Pledge: I certify that no unauthorized assistance has been received or
  * given in the completion of this work
  */

// Library Includes
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h> // mmap, munmap
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h> // kill
#include <fcntl.h> // O_CREAT
#include <semaphore.h> // POSIX semaphores Library
// Custom includes
#include "helper.h"
#include "buffer.h"
#define SEMAPHORE_NAME "/sharedSem"
#define ERROR -1
// Function prototypes for each process
void streamToCharConversion(Buffer* mmap);
void carriageReturnToBlankConversion(Buffer* mmap);
void squashAsterisks(Buffer* mmap);
void consumer(Buffer* mmap);
// Other function prototypes
pid_t forkChild(void (*func)(Buffer *), Buffer* state);
void waitForChildren(pid_t*);
int main(int argc, char const *argv[])
{
  Buffer* mmapFile = createMMAP(sizeof(Buffer));
  // fork children
  pid_t childpids[2];
  childpids[0] = forkChild(streamToCharConversion, mmapFile);
  childpids[1] = forkChild(consumer, mmapFile);

  //wait for them
  waitForChildren(childpids);
  // cleanup
  deleteMMAP(mmapFile);
  exit(EXIT_SUCCESS);
}


pid_t forkChild(void (*function) (Buffer *), Buffer* state) {
    // This function takes a pointer to a function as an argument.
    // It returns the forked child's PID
    pid_t childPID;
    switch (childPID = fork()) {
        case ERROR:
            perror("fork error");
            exit(EXIT_FAILURE);
        case 0:
            (*function) (state);
        default:
            return childPID;
    }
}

void waitForChildren(pid_t* childpids){
	int status;
	while (ERROR < wait(&status)){ //Here the parent waits on any child.
		if (!WIFEXITED(status)){ //If the termination err, kill all children.
			kill(childpids[0], SIGKILL);
	 		kill(childpids[1], SIGKILL);
			break;
	 	}
	}
}

void streamToCharConversion(Buffer* mmap) {
    // reads a char from stdin & write it to mem mapped file
    char c = fgetc(stdin);
    deposit(c, mmap);
    exit(EXIT_SUCCESS); 
}

void consumer(Buffer* mmap) {
    // reads a char from mem mapped file & print it
    char c;
    remoove(&c, mmap);
    putchar(c);
    putchar('\n');
    exit(EXIT_SUCCESS);
}
