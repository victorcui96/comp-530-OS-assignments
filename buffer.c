#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "helper.h"
#include "buffer.h"
Buffer* createMMAP(size_t size) {
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
  state->fullBuffers = sem_open(SEMAPHORE_NAME, O_CREAT, S_IREAD | S_IWRITE, 0);
  state->emptyBuffers = sem_open(SEMAPHORE_NAME, O_CREAT, S_IREAD | S_IWRITE, 1);
  return state;
}  
// Producer process
void deposit(char c, Buffer* dest) {
    sem_wait(&(dest->emptyBuffers));
    dest->content[0] = c;
    sem_post(&(dest->fullBuffers));
}
// Consumer process
void remove(char* data, Buffer* src) {
  sem_wait(&(src->fullBuffers));
  *data = src->content[0];
  sem_post(&(src->emptyBuffers));
}
void deleteMMAP(void* addr) {
  // This deletes the memory map at given address. see man mmap
  if (ERROR == munmap(addr, sizeof(Buffer)))
  {
    perror("error deleting mmap");
    exit(EXIT_FAILURE);
  }
}

