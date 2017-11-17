#include "helper.h"
// Object mapped to memory.It has an array and a count of objects in the array. 
typedef struct Buffer {
  char content[OUTPUT_LEN];
  int count;
} Buffer; // Defines struct "Buffer" within struct name space
