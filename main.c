#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>


#include "constants.h"
#include "parser.h"
#include "operations.h"
#include "fileManipulation.h"

sem_t thread_semaphore; 

int count_threads = 0;

typedef struct {

  int thread_id;
  char file[MAX_PATH + MAX_WRITE_SIZE];

} thread_data;

void* thread_process_file(void* arg) {

    thread_data* t = (thread_data*)arg; // Converte o argumento para o tipo correto
    printf("[Thread Start] Thread ID: %d  File: %s\n",t->thread_id, t->file);
    process_file(t->file); // Função já existente no seu código
    
    sleep(2);

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
  //char cwd[MAX_PATH]; //Buffer to Store Directories
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
  cleanup_semaphore();
  
  return 0;
}
