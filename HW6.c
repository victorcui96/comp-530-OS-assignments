 /*******************************
  * AUTHOR: Victor Cui
  * PROBLEM DESCRIPTION: Implement HW1 using a bounded buffer ADT, memory-mapped files, and POSIX semaphores to synchronize
  * accesses to memory mapped files
  *
  * SOLUTION APPROACH: Wrap a memory mapped file inside a buffer ADT, and give each producer / consumer pair of processes a read buffer and 
  * a write buffer. Use POSIX semaphores to synchronize accesses to memory mapped files.
  *
  * UNC Honor Pledge: I certify that no unauthorized assistance has been received or
  * given in the completion of this work
  *
  * All online resources / sites considered: 
  * http://www.csc.villanova.edu/~mdamian/threads/posixsem.html
  * http://www.geeksforgeeks.org/use-posix-semaphores-c/
  * https://en.wikipedia.org/wiki/Memory-mapped_file
  */
 
 /* How I'm using memory mapped files: I wrap the implementation of a memory mapped file inside my Buffer ADT.
  When I initialize a portion of memory belonging to a Buffer, the memory mapped file is created along with it. I
  then read and write to the memory mapped file as I please. The functions deposit and remove write and remove characters
  from a memory mapped file, respectively. A memory mapped file uses two counting semaphores to coordinate 
  synchronization. I delete the MMAP files when all the processes in my pipeline have completed. 
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
// Constants
#define ERROR -1
#define FILENAME "tmpFileForMapping"
#define EMPTY_BUFFERS_SEM_1 "/emptyBuffers1"
#define FULL_BUFFERS_SEM_1 "/fullBuffers1"
#define EMPTY_BUFFERS_SEM_2 "/emptyBuffers2"
#define FULL_BUFFERS_SEM_2 "/fullBuffers2"
#define EMPTY_BUFFERS_SEM_3 "/emptyBuffers3"
#define FULL_BUFFERS_SEM_3 "/fullBuffers3"
// Function prototypes for each process
void streamToCharConversion(Buffer* writeBuffer);
void carriageReturnToBlankConversion(Buffer* readBuffer, Buffer* writeBuffer);
void squashAsterisks(Buffer* readBuffer, Buffer* writeBuffer);
void consumer(Buffer* readBuffer);
// Other function prototypes
pid_t forkChild(void (*func)(Buffer *), Buffer* state);
pid_t forkChildToHandleNewlineConversion(Buffer* readBuf, Buffer* writeBuf);
pid_t forkChildToSquashingAsterisks(Buffer* readBuf, Buffer* writeBuf);
void waitForChildren(pid_t*);

int main(int argc, char const *argv[])
{
  // Create memory mapped files
  Buffer* mmapFile1 = createMMAP(sizeof(Buffer), EMPTY_BUFFERS_SEM_1, FULL_BUFFERS_SEM_1);
  Buffer* mmapFile2 = createMMAP(sizeof(Buffer), EMPTY_BUFFERS_SEM_2, FULL_BUFFERS_SEM_2);
  Buffer* mmapFile3 = createMMAP(sizeof(Buffer), EMPTY_BUFFERS_SEM_3, FULL_BUFFERS_SEM_3);
  // fork children
  pid_t childpids[3];
  childpids[0] = forkChild(streamToCharConversion, mmapFile1);
  childpids[1] = forkChildToHandleNewlineConversion(mmapFile1, mmapFile2);
  childpids[2] = forkChildToSquashingAsterisks(mmapFile2, mmapFile3);
  childpids[3] = forkChild(consumer, mmapFile3);
  //wait for them
  waitForChildren(childpids);
  // cleanup
  deleteMMAP(mmapFile1);
  deleteMMAP(mmapFile2);
  deleteMMAP(mmapFile3);
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

/* fork the process used to handle new line -> space conversions */
pid_t forkChildToHandleNewlineConversion(Buffer* readBuffer, Buffer* writeBuffer) {
    pid_t childPID;
    switch (childPID = fork()) {
        case ERROR:
            perror("fork error");
            exit(EXIT_FAILURE);
        case 0:
          carriageReturnToBlankConversion(readBuffer, writeBuffer);
        default:
          return childPID;
    }
}
/* fork the process used the squash adjacent characters in the input stream buffer */
pid_t forkChildToSquashingAsterisks(Buffer* readBuf, Buffer* writeBuf) {
    pid_t childPID;
    switch (childPID = fork()) {
        case ERROR:
            perror("fork error");
            exit(EXIT_FAILURE);
        case 0:
          squashAsterisks(readBuf, writeBuf);
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
/* Read from stdin and generate a stream of characters, until EOF */
void streamToCharConversion(Buffer* mmap) {
    while (1) {
      char c = getchar();
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
void carriageReturnToBlankConversion(Buffer* readBuffer, Buffer* writeBuffer) {
  char c;
  while (1) {
    remoove(&c, readBuffer);
    if (c == '\n')
    {
      c = ' ';
    }
    deposit(c, writeBuffer);
    if (c == EOF)
    {
      break;
    }
  }
}
/* Converts consecutive asterisks -> a carat */
void squashAsterisks(Buffer* readBuffer, Buffer* writeBuffer) {
  bool seenAsterikPreviousky = false;
  char charInBuffer;
  while (1) {
    remoove(&charInBuffer, readBuffer);
    if (seenAsterikPreviousky) {
      // uses current char and seen asterisk previously glag to determine output to producer buffer
      if (charInBuffer == '*') {
        deposit('^', writeBuffer);
      } else {
        deposit('*', writeBuffer);
      }
      seenAsterikPreviousky = false;
    } else if (charInBuffer == '*') {
        seenAsterikPreviousky = true;
    } else {
      deposit(charInBuffer, writeBuffer);
    }
    if (charInBuffer == EOF) {
      break;
    }
  }
}
/* Prints out 80 character lines to stdout */
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
