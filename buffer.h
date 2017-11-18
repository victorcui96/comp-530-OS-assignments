#include "helper.h"
// Object mapped to memory.It has an array and a count of objects in the array. 
typedef struct Buffer {
  char content[OUTPUT_LEN];
  int count;
  sem_t* fullBuffers;
  sem_t* emptyBuffers;
  const char* emptyBuffersSem;
  const char* fullBuffersSem;
  
} Buffer; // Defines struct "Buffer" within struct name space

Buffer* createMMAP(size_t size);
void deposit(char c, Buffer* dest);
void remoove(char* data, Buffer* src);
void deleteMMAP(Buffer* addr);
