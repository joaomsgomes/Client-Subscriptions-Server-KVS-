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

sem_t *backup_sem; // Semáforo global para controlar backups

int ongoingbackups;

// Inicializa o semáforo no processo principal
void initialize_semaphore(int maxBackups) {
    backup_sem = sem_open("/backup_sem", O_CREAT, 0644, maxBackups);
    if (backup_sem == SEM_FAILED) {
        perror("Failed to create semaphore");
        exit(1);
    }
}

// Fecha e remove o semáforo
void cleanup_semaphore() {
    sem_close(backup_sem);
    sem_unlink("/backup_sem");
}

void wait_for_backup_slot() {
    sem_wait(backup_sem); // Decrementa o contador do semáforo
}

void release_backup_slot() {
    sem_post(backup_sem); // Incrementa o contador do semáforo
}

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

// Função de comparação para ordenar as chaves em ordem alfabética
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

  // Criação de um vetor de ponteiros para as chaves
  char *sorted_keys[num_pairs];
  for (size_t i = 0; i < num_pairs; i++) {
    sorted_keys[i] = keys[i];
  }

  // Ordenação das chaves
  qsort(sorted_keys, num_pairs, sizeof(char*), compare_keys);

  strcat(output, "[");

  for (size_t i = 0; i < num_pairs; i++) {
    // Obtém o valor da chave
    char* result = read_pair(kvs_table, sorted_keys[i]);

    // Diagnóstico: verificar o valor de result
    if (result == NULL) {
      snprintf(output_temp, MAX_WRITE_SIZE, "(%s,KVSERROR)", sorted_keys[i]);
      strcat(output, output_temp);
    } else {
      snprintf(output_temp, MAX_WRITE_SIZE, "(%s,%s)", sorted_keys[i], result);
      strcat(output, output_temp);
      
      // Certifique-se de que result deve ser liberado
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
  // printf("output: %s\n", output);

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

int kvs_backup(const char* file_path, int backupCounter) {

    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed for backup");
        release_backup_slot(); // Libera o slot no caso de erro
        return -1; // Erro ao criar o processo
    }

    if (pid == 0) {

        char *file_path_no_ext = remove_extension(file_path, ".job");
        char aux_path[MAX_WRITE_SIZE];                        
        snprintf(aux_path, MAX_PATH + MAX_WRITE_SIZE, "%s-%d", file_path_no_ext, backupCounter);
        // printf("filePathNoExtension: %s\n", file_path_no_ext);

        free(file_path_no_ext);
        char* backup_file_path = add_extension(aux_path, ".bck");

        printf("Process %d: Attempting to acquire backup slot...\n", getpid());
        int slots_available;
        sem_getvalue(backup_sem, &slots_available);
        printf("Slots available: %d\n", slots_available);

        wait_for_backup_slot(); // Aguarda vaga para backup
        printf("XX Process %d: Acquired backup slot. Starting backup...\n", getpid());


        sleep(1);

        // Código do processo filho
        char output[MAX_WRITE_SIZE];
        kvs_show(output); // Gera a tabela
        int fBackup = open(backup_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fBackup < 0) {
            perror("Failed to open backup file");
            _exit(1); // Saída com erro
        }

        write_in_file(output, fBackup); // Escreve os dados no arquivo
        close(fBackup);

        printf("Process %d: Backup completed: %s\n", getpid(), backup_file_path);
        printf("OO Process %d: Backup finished. Releasing slot...\n\n", getpid());
        release_backup_slot(); // Libera o slot após o término do backup
        free(backup_file_path);
        _exit(0); // Termina o filho corretamente
    }

    // Código do processo pai
    //waitpid(pid, NULL, 0); // Aguarda o filho terminar
    //printf("OO Process %d: Backup finished. Releasing slot...\n\n", getpid());
    // release_backup_slot(); // Libera o slot após o término do backup

    return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}