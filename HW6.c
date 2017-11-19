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
#include <stdbool.h>
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
#define ERROR -1
#define FILENAME "tmpFileForMapping"
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
  pid_t childpids[3];
  childpids[0] = forkChild(streamToCharConversion, mmapFile);
  childpids[1] = forkChild(carriageReturnToBlankConversion, mmapFile);
  childpids[2] = forkChild(squashAsterisks, mmapFile);
  childpids[3] = forkChild(consumer, mmapFile);

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
/* Read from stdin and generate a stream of characters, until array is full */
void streamToCharConversion(Buffer* mmap) {
    // reads a char from stdin & writes to mem mapped file, until array is full */
    while (1) {
      char c = getchar();
      // printf("char seen from input: %c\n", c);
      deposit(c, mmap);
      if (c == EOF)
      {
        // exit if we see EOF in the buffer
        break;
      }
    }
    exit(EXIT_SUCCESS);
}

/* Converts carriage returns to spaces */
void carriageReturnToBlankConversion(Buffer* mmap) {
  char c;
  while (1) {
    remoove(&c, mmap);
    if (c == '\n')
    {
      c = ' ';
    }
    // printf("char seen in carriage return -> blank function: %c\n", c);
    deposit(c, mmap);
    if (c == EOF)
    {
      break;
    }
  }
}
/* Converts consecutive asterisks -> a carat */
void squashAsterisks(Buffer* mmap) {
  bool seenAsterikPreviousky = false;
  char charInBuffer;
  while (1) {
    remoove(&charInBuffer, mmap);
    if (seenAsterikPreviousky)
    {
      // uses current char and seen asterisk previously glag to determine output to producer buffer
      if (charInBuffer == '*') {
        deposit('^', mmap);
      } else {
        deposit('*', mmap);
      }
    }
    else if (charInBuffer == '*') {
      seenAsterikPreviousky = true;
    } else {
      deposit(charInBuffer, mmap);
    }
    if (charInBuffer == EOF) {
      break;
    }
  }
}

void consumer(Buffer* mmap) {
    // reads a char from mem mapped file & print it
    char c;
    // counts number of elements seen in array
    int count = 0;
    // array that we will print out
    char output[OUTPUT_LEN];
    while (1) {
      remoove(&c, mmap);
      if (c == EOF)
      {
        break;
      }
      output[count] = c;
      // printf("char seen in output: %c\n", c);
      count++;
      if (count == OUTPUT_LEN)
      {
        // print all characters in output if output is full
        for (int i = 0; i < OUTPUT_LEN; ++i)
        {
          printf("%c", output[i]);
        }
        printf("\n");
        count = 0;
      }
    }
    exit(EXIT_SUCCESS);
}
