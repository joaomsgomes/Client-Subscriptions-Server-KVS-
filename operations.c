#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>

#include "kvs.h"
#include "constants.h"
#include "fileManipulation.h"

static struct HashTable* kvs_table = NULL;

int ongoingBackups = 0;

/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

int kvs_init() {
  if (kvs_table != NULL) {
    fprintf(stderr, "KVS state has already been initialized\n");
    return 1;
  }

  kvs_table = create_hash_table();
  return kvs_table == NULL;
}

int kvs_terminate() {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  free_table(kvs_table);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }

  return 0;
}

int compare_keys(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], char output[MAX_WRITE_SIZE]) {
  
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  memset(output, 0, MAX_WRITE_SIZE);

  char output_temp[MAX_WRITE_SIZE];

  
  char *sorted_keys[num_pairs];
  for (size_t i = 0; i < num_pairs; i++) {
    sorted_keys[i] = keys[i];
  }

  
  qsort(sorted_keys, num_pairs, sizeof(char*), compare_keys);

  strcat(output, "[");

  for (size_t i = 0; i < num_pairs; i++) {
    
    char* result = read_pair(kvs_table, sorted_keys[i]);

    
    if (result == NULL) {
      snprintf(output_temp, MAX_WRITE_SIZE, "(%s,KVSERROR)", sorted_keys[i]);
      strcat(output, output_temp);
    } else {
      snprintf(output_temp, MAX_WRITE_SIZE, "(%s,%s)", sorted_keys[i], result);
      strcat(output, output_temp);
      
      
      free(result);
    }
  }

  strcat(output, "]\n");
  
  return 0;
}

int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], char output[MAX_WRITE_SIZE]) {

  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  int aux = 0;

  memset(output, 0, MAX_WRITE_SIZE);

  char output_temp[MAX_WRITE_SIZE];

  for (size_t i = 0; i < num_pairs; i++) {
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        strcat(output, "[");
        aux = 1;
      }
      snprintf(output_temp, MAX_WRITE_SIZE, "(%s,KVSMISSING)", keys[i]);
      strcat(output, output_temp);
    }
  }
  if (aux) {
    strcat(output, "]\n");
  }
  

  return 0;
}

void kvs_show(char output[MAX_WRITE_SIZE]) {
    
  memset(output, 0, MAX_WRITE_SIZE);

  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
        
      strcat(output, "(");
      strcat(output, keyNode->key);
      strcat(output, ", ");
      strcat(output, keyNode->value);
      strcat(output, ")\n");
  

      keyNode = keyNode->next;
    }
  }
}

int kvs_backup(const char* file_path, int backupCounter, int maxBackups) {

    if (ongoingBackups == maxBackups) {
      printf("Backup number %d is waiting for a backup to end...\n", backupCounter);
      printf("ONGOING BACKUPS: %d\n", ongoingBackups);
      wait(NULL);
      ongoingBackups--;
    }

    ongoingBackups++;
    printf("Process %d: Starting Backup: %s-%d\n", getpid(), file_path, backupCounter);
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed for backup");
        ongoingBackups--;
        return -1; 
    }

    if (pid == 0) {

        char *file_path_no_ext = remove_extension(file_path, ".job");
        char aux_path[MAX_WRITE_SIZE];                        
        snprintf(aux_path, MAX_PATH + MAX_WRITE_SIZE, "%s-%d", file_path_no_ext, backupCounter);
        
        //sleep(1);
        free(file_path_no_ext);
        char* backup_file_path = add_extension(aux_path, ".bck");
              
        char output[MAX_WRITE_SIZE];
        kvs_show(output); 
        int fBackup = open(backup_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fBackup < 0) {
            perror("Failed to open backup file");
            _exit(1); 
        }

        write_in_file(output, fBackup); 
        close(fBackup);

        printf("Process %d: Backup completed: %s\n", getpid(), backup_file_path);
        
        free(backup_file_path);
        _exit(0);
    }

    return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}