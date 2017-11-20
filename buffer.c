/* Buffer ADT implementation */
#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> // O_CREAT
#include <semaphore.h>
#include "buffer.h"
// Constants
#define EMPTY_BUFFERS_SEM "/emptyBuffers"
#define FULL_BUFFERS_SEM "/fullBuffers"
Buffer* createMMAP(size_t size, const char* emptyBuffersSem, const char* fullBuffersSem) {
  // These are the neccessary arguments for mmap. See man mmap.
  void* addr = 0;
  int protections = PROT_READ|PROT_WRITE; //can read and write
  int flags = MAP_SHARED | MAP_ANONYMOUS; // if a process updates the file, want the update to be shared among all other processes
  int fd = -1;
  off_t offset = 0;
  // Create memory map
  Buffer* state =  mmap(addr, size, protections, flags, fd, offset);
  if (( void *) ERROR == mmap) {
    // on an error mmap returns void* -1.
    perror("error with map");
    exit(EXIT_FAILURE);
  }
  state->emptyBuffersSem = emptyBuffersSem;
  state->fullBuffersSem = fullBuffersSem;
  // open the semaphore we need to use
  // O_CREAT specifies that the semaphore should be created if it does not already exist
  // S_IREAD | S_IWRTIE gives us read & write permissions on the semaphore
  // Initial value of semaphore = 0
  state->fullBuffers = sem_open(emptyBuffersSem, O_CREAT, S_IREAD | S_IWRITE, 0);
  // open the semaphore we need to use
  // O_CREAT specifies that the semaphore should be created if it does not already exist
  // S_IREAD | S_IWRTIE gives us read & write permissions on the semaphore
  // Initial value of semaphore = 1
  state->emptyBuffers = sem_open(fullBuffersSem, O_CREAT, S_IREAD | S_IWRITE, OUTPUT_LEN);
  return state;
}  
// Producer process
void deposit(char c, Buffer* dest) {
    sem_wait(dest->emptyBuffers);
    dest->content[dest->count] = c;
    dest->count = ((dest->count + 1) % (OUTPUT_LEN));
    sem_post(dest->fullBuffers);
}
// Consumer process
void remoove(char* data, Buffer* src) {
  // prints lines to stdout
  sem_wait(src->fullBuffers);
  *data = src->content[src->count];
  src->count--;
  sem_post(src->emptyBuffers);
}
void deleteMMAP(Buffer* addr) {
  // any semaphores we used must be unlinked
  // sem_unlink removes semaphore from system once every process that has used the semaphore has closed it
  if (sem_unlink(addr->fullBuffersSem) == ERROR || sem_unlink(addr->emptyBuffersSem) == ERROR) {
    perror("Error while unlinking semaphore");
    exit(EXIT_FAILURE);
  }
  // This deletes the memory map at given address. see man mmap
  if (ERROR == munmap(addr, sizeof(Buffer)))
  {
    perror("error deleting mmap");
    exit(EXIT_FAILURE);
  } 
}

