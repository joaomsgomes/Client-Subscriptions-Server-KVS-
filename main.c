#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h> 
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>


#include "constants.h"
#include "parser.h"
#include "operations.h"

// Struct for thread data
int threads_created = 0;

typedef struct {

  int thread_id;
  char file[MAX_PATH + MAX_WRITE_SIZE];
  int active;

} thread_data;


int maxBackups = 0;
int maxThreads = 0;

// Function to process commands on a file
int process_file(thread_data* t_data) {


    // Open commands file and create output file
    int fd = open(t_data->file,O_RDONLY); 
    

    if (fd < 0) {
        
        perror("Error opening file\n");
    }
    
    char* out_file_path = modify_file_path(t_data->file, ".job",".out");
    
    int fd_out = open(out_file_path,O_WRONLY | O_CREAT | O_TRUNC, 0644);
    

    if (fd_out < 0) {
        
        perror("Error opening file\n");
    }

    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    unsigned int delay;
    size_t num_pairs;
    char output[MAX_WRITE_SIZE];
    int backupCounter = 0;
    
    while (1) {
        
        switch (get_next(fd)) {

            case CMD_WRITE:

                num_pairs = parse_write(fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);

                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    break;
                }

                if (kvs_write(num_pairs, keys, values)) {
                    fprintf(stderr, "Failed to write pair\n");
                }

                break;

            case CMD_READ:

                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
    
                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    break;
                }

                if (kvs_read(num_pairs, keys, output)) {
                    fprintf(stderr, "Failed to read pair\n");
                }
                write_in_file(output, fd_out);


                break;

            case CMD_DELETE:

                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    break;
                }

                if (kvs_delete(num_pairs, keys, output)) {
                    fprintf(stderr, "Failed to delete pair\n");
                }

                write_in_file(output, fd_out);

                break;

            case CMD_SHOW:

                kvs_show(output);
                write_in_file(output, fd_out);

                break;

            case CMD_WAIT:
                if (parse_wait(fd, &delay, NULL) == -1) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    break;
                }

                if (delay > 0) {
                    
                    strcpy(output, "Waiting...\n");
                    write_in_file(output, fd_out);
                    

                    kvs_wait(delay); 
                }

                break;

            case CMD_BACKUP:


                backupCounter++;
              
                if (kvs_backup(t_data->file, backupCounter, maxBackups)) {
                    fprintf(stderr, "Failed to perform backup.\n");
                }


                break;

            case CMD_INVALID:
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                break;

            case CMD_HELP:
                printf( 
                    "Available commands:\n"
                    "  WRITE [(key,value)(key2,value2),...]\n"
                    "  READ [key,key2,...]\n"
                    "  DELETE [key,key2,...]\n"
                    "  SHOW\n"
                    "  WAIT <delay_ms>\n"
                    "  BACKUP\n" 
                    "  HELP\n"
                );

                break;
                
            case CMD_EMPTY:
                break;

            case EOC:

                wait(NULL);
                t_data->active = 0;
                free(out_file_path);
                close(fd);
                close(fd_out);
                return 0;
        }
    }

}

void* thread_process_file(void* arg) {

    thread_data* t = (thread_data*)arg;
    
    process_file(t);

    return NULL;
}


int readFiles(char* path) {

    thread_data* threads_data[maxThreads];
    pthread_t threads[maxThreads];

    

    for (int i = 0; i < maxThreads; i++ ) {

        thread_data* t = malloc(sizeof(thread_data));
        t->thread_id = i;
        t->active = 1;
        threads_data[i] = t;
        
    }

    struct dirent *file;
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("Error Opening Directory\n");
        return -1; 
    }

    size_t path_len = strlen(path);
    char aux_path[path_len + MAX_PATH];
    int i = 0;

    while ((file = readdir(dir)) != NULL) {
        
        if (is_job_file(file->d_name)) {
            
            snprintf(aux_path, sizeof(aux_path), "%s/%s", path, file->d_name);
            
            
            
            if (threads_created < maxThreads) {
                
                strcpy(threads_data[threads_created]->file, aux_path);
                if (pthread_create(&threads[threads_created], NULL, thread_process_file, threads_data[threads_created]) != 0) {
                    perror("Failed to create thread\n");
                    continue;
                }
                threads_created++;
                
            } else {
                while(i < maxThreads) {
                    if (!threads_data[i]->active) {
                        
                        strcpy(threads_data[i]->file, aux_path);
                        threads_data[i]->active = 1;

                        
                        process_file(threads_data[i]);
                        break;
                    }

                    i++;
                    if(i == maxThreads) {
                        i = 0;
                    }
                }
                
            }
                
        }

    }
    for (int j = 0; j < maxThreads; j++) {
        pthread_join(threads[j], NULL);
        free(threads_data[j]);
    }
    closedir(dir);  
    return 0;
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Invalid Number of Arguments\n");
    return 0;
  }

  char* dir = argv[1]; 
  maxBackups = atoi(argv[2]); 
  maxThreads = maxBackups; 


  if (maxThreads <= 0) {
        fprintf(stderr, "Invalid thread limit\n");
        return 1;
  }

  

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  readFiles(dir);  

  return 0;
}
