void createMMAP(Buffer* state, size_t size) {
  // These are the neccessary arguments for mmap. See man mmap.
  void* addr = 0;
  int protections = PROT_READ|PROT_WRITE; //can read and write
  int flags = MAP_SHARED; // if a process updates the file, want the update to be shared among all other processes
  int fd = -1;
  off_t offset = 0;
  // Create memory map
  state =  mmap(addr, size, protections, flags, fd, offset);
  if (( void *) ERROR == mmap) {
    // on an error mmap returns void* -1.
    perror("error with map");
    exit(EXIT_FAILURE);
  }
}

