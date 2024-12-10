#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "constants.h"
#include "parser.h"
#include "operations.h"
#include "fileManipulation.h"

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Invalid Number of Arguments\n");
    return 0;
  }

  char* dir = argv[1]; 
  int maxBackups = atoi(argv[2]);
  //char cwd[MAX_PATH]; //Buffer to Store Directories
  
  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  readFiles(dir, maxBackups);

  return 0;
}
