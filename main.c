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
#include <semaphore.h>


#include "constants.h"
#include "parser.h"
#include "operations.h"
#include "fileManipulation.h"

sem_t thread_semaphore;
pthread_mutex_t mutex;

pthread_rwlock_t rwlock;

int count_threads = 0;

typedef struct {

  int thread_id;
  char file[MAX_PATH + MAX_WRITE_SIZE];

} thread_data;



int process_file(const char* file_path) {

    int fd = open(file_path,O_RDONLY); 
    //printf("Opening file Thread ID: %s %lu\n", file_path, thread_self());

    if (fd < 0) {
        printf("Error number: %d\n", errno);
        perror("Error opening file\n");
    }
    //printf("vai modificar o ficheiro\n");
    char* out_file_path = modify_file_path(file_path, ".job",".out");
    //printf("abrir fd: %s\n", out_file_path);
    int fd_out = open(out_file_path,O_WRONLY | O_CREAT | O_TRUNC, 0644);
    //printf("Opening output file: %s\n", out_file_path);

    if (fd_out < 0) {
        printf("Error number: %d\n", errno);
        perror("Error opening file\n");
    }

    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    unsigned int delay;
    size_t num_pairs;
    char output[MAX_WRITE_SIZE];
    int backupCounter = 0;
    
    while (1) {
        //printf("newCommand\n");
        switch (get_next(fd)) {

            case CMD_WRITE:

                pthread_rwlock_wrlock(&rwlock);

                num_pairs = parse_write(fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);

                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    pthread_rwlock_unlock(&rwlock);
                    break;
                }

                if (kvs_write(num_pairs, keys, values)) {
                    fprintf(stderr, "Failed to write pair\n");
                }

                pthread_rwlock_unlock(&rwlock);

                break;

            case CMD_READ:
                
                pthread_rwlock_rdlock(&rwlock);

                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
                //printf("NumPairs: %ld\n", num_pairs);
                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    pthread_rwlock_unlock(&rwlock);
                    break;
                }

                if (kvs_read(num_pairs, keys, output)) {
                    fprintf(stderr, "Failed to read pair\n");
                }
                write_in_file(output, fd_out);
                
                pthread_rwlock_unlock(&rwlock);

                break;

            case CMD_DELETE:

                pthread_rwlock_wrlock(&rwlock); // Adquire lock de escrita.

                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    pthread_rwlock_unlock(&rwlock); // Libera lock.
                    break;
                }

                if (kvs_delete(num_pairs, keys, output)) {
                    fprintf(stderr, "Failed to delete pair\n");
                }

                write_in_file(output, fd_out);

                pthread_rwlock_unlock(&rwlock); // Libera lock após a operação.

                break;

            case CMD_SHOW:

                pthread_rwlock_rdlock(&rwlock);

                kvs_show(output);
                write_in_file(output, fd_out);

                pthread_rwlock_unlock(&rwlock);

                break;

            case CMD_WAIT:
                if (parse_wait(fd, &delay, NULL) == -1) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    break;
                }

                if (delay > 0) {
                    pthread_mutex_lock(&mutex); // Protege o acesso a `output`.
                    strcpy(output, "Waiting...\n");
                    write_in_file(output, fd_out);
                    pthread_mutex_unlock(&mutex); // Libera o acesso a `output`.

                    kvs_wait(delay); // Executa a espera sem acessar `output`.
                }

                break;

            case CMD_BACKUP:

                
                pthread_rwlock_wrlock(&rwlock); // Adquire lock de escrita.

                backupCounter++;
                if (kvs_backup(file_path, backupCounter)) {
                    fprintf(stderr, "Failed to perform backup.\n");
                }

                pthread_rwlock_unlock(&rwlock); // Libera lock.


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
                // kvs_terminate();
                wait(NULL);
                free(out_file_path);
                close(fd);
                close(fd_out);
                return 0;
        }
    }

}

void* thread_process_file(void* arg) {

    thread_data* t = (thread_data*)arg; // Converte o argumento para o tipo correto
    printf("[Thread Start] Thread ID: %d  File: %s\n",t->thread_id, t->file);
    process_file(t->file); // Função já existente no seu código
    
    //sleep(1);

    sem_post(&thread_semaphore); // Libera o slot do semáforo após finalizar
    //printf("[Semaphore] Slot released for file: %s\n", file);
    printf("[Thread End] Thread ID: %d  File: %s\n", t->thread_id, t->file);
    free(t);
    
    return NULL;
}




int readFiles(char* path) {

    struct dirent *file;
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("Error Opening Directory\n");
        return -1; 
    }

    size_t path_len = strlen(path);
    char aux_path[path_len + MAX_PATH];
    pthread_t thread;

    while ((file = readdir(dir)) != NULL) {
        
        if (is_job_file(file->d_name)) {
            printf("fileName: %s\n", file->d_name);
            snprintf(aux_path, sizeof(aux_path), "%s/%s", path, file->d_name);
            //printf("filePath: %s\n", aux_path);

            sem_wait(&thread_semaphore); // Bloqueia se não houver slots disponíveis
            //printf("[Semaphore] Slot acquired for file: %s\n", aux_path);
            count_threads++;
            thread_data* t = malloc(sizeof(thread_data));

            t->thread_id = count_threads;
            strcpy(t->file, aux_path);

            if (pthread_create(&thread, NULL, thread_process_file,t) != 0) {
                perror("Failed to create thread\n");
                sem_post(&thread_semaphore); // Libera o slot no caso de falha
                continue;
            }
            
        }
    }
    pthread_join(thread, NULL);
    closedir(dir);  // Fecha o diretório após a leitura
    return 0;
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Invalid Number of Arguments\n");
    return 0;
  }

  char* dir = argv[1]; 
  int maxBackups_threads = atoi(argv[2]); 

  pthread_mutex_init(&mutex, NULL);
  pthread_rwlock_init(&rwlock, NULL);


  initialize_semaphore(maxBackups_threads);

  if (maxBackups_threads <= 0) {
        fprintf(stderr, "Invalid thread limit\n");
        return 1;
  }

  if (sem_init(&thread_semaphore, 0,(unsigned int) maxBackups_threads) != 0) {
        perror("Failed to initialize semaphore\n");
        return 1;
  }

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    sem_destroy(&thread_semaphore);
    return 1;
  }

  readFiles(dir);

  pthread_mutex_destroy(&mutex);
  pthread_rwlock_destroy(&rwlock);


  cleanup_semaphore();

  return 0;
}
