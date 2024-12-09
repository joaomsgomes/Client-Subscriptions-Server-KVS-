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
  char cwd[PATH_MAX]; //Buffer to Store Directories

  char* currDir = getcwd(cwd, sizeof(cwd));
  printf("Current Directory: %s  maxBackups: %d Directory: %s\n", currDir, maxBackups, dir);
  
  readFiles(dir);
  

  currDir = getcwd(cwd, sizeof(cwd));

  
  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  return 0;
}
