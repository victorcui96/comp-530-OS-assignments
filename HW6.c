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
// void deleteMMAP(void*);
// get address of memory mapped location.
Buffer* mmapFile;
int main(int argc, char const *argv[])
{
  mmapFile = createMMAP(sizeof(Buffer));
  // fork children
  pid_t childpids[2];
  childpids[0] = forkChild(streamToCharConversion, mmapFile);
  childpids[1] = forkChild(consumer, mmapFile);

  //wait for them
  waitForChildren(childpids);
  // any semaphores we used must be unlinked
  // sem_unlink removes semaphore from system once every process that has used the semaphore has closed it
  if (sem_unlink(SEMAPHORE_NAME) == ERROR) {
      perror("error while linking semaphore.");
      exit(EXIT_FAILURE);
  }
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
    // Read a char from stdin & writes it until array is full
    // open the semaphore we need to use
    // SEMAPHORE_NAME is the name of the semaphore to open
    // O_CREAT specifies that the semaphore should be created if it does not already exist
    // S_IREAD | S_IWRTIE gives us read & write permissions on the semaphore
    // Initial value of semaphore = 0
    sem_t* sem = sem_open(SEMAPHORE_NAME, O_CREAT, S_IREAD | S_IWRITE, 0);
    if (sem == SEM_FAILED) {
        perror("could not open semaphore");
        exit(EXIT_FAILURE); 
    }
    // reads a char from stdin & write it to mem mapped file
    char c = fgetc(stdin);
    mmap->content[0] = c;
    // increment value of sem by calling sem_post -> up() from lecture
    if (sem_post(sem) == ERROR) {
        perror("error incrementing semaphore");
        exit(EXIT_FAILURE);
    }
    // close the semaphore we opened
    if (sem_close(sem) == ERROR) {
        perror("Error closing semaphore.");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS); 
}

void consumer(Buffer* mmap) {
    // Read a char from stdin & writes it until array is full
    // open the semaphore we need to use
    // SEMAPHORE_NAME is the name of the semaphore to open
    // O_CREAT specifies that the semaphore should be created if it does not already exist
    // S_IREAD | S_IWRTIE gives us read & write permissions on the semaphore
    // Initial value of semaphore = 0
    sem_t* sem = sem_open(SEMAPHORE_NAME, O_CREAT, S_IREAD | S_IWRITE, 0);
    if (sem == SEM_FAILED) {
        perror("could not open semaphore");
        exit(EXIT_FAILURE); 
    }
    // wait on sem -> down() from lecture
    if (sem_wait(sem) == ERROR) {
        perror("error while waiting on semaphore");
        exit(EXIT_FAILURE);
    } 
    // reads a char from mem mapped file & print it
    char c = mmap->content[0];
    // close the sem we opened
    if (sem_close(sem) == ERROR) {
        perror("Error closing semaphore.");
        exit(EXIT_FAILURE);
    }
    putchar(c);
    putchar('\n');
    exit(EXIT_SUCCESS);
}
