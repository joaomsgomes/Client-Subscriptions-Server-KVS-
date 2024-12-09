#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "kvs.h"
#include "constants.h"
#include "fileManipulation.h"

static struct HashTable* kvs_table = NULL;


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
  printf("Output KVS DELETE: %s\n", output);
  memset(output, 0, MAX_WRITE_SIZE);
  printf("Output KVS DELETE: %s\n", output);

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

int kvs_backup(const char* backup_file_path) {
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed for backup");
        return -1; // Erro ao criar o processo
    }

    if (pid == 0) { // Processo Filho
        char output[MAX_WRITE_SIZE];
        kvs_show(output); // Gera a tabela
        printf("Backup FILEPATH: %s\n", backup_file_path);
        int fBackup = open(backup_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fBackup < 0) {
            perror("Failed to open backup file");
            _exit(1); // Saída com erro
        }

        write_in_file(output, fBackup); // Escreve os dados no arquivo
        close(fBackup);
        
        printf("Backup completed: %s\n", backup_file_path);
        _exit(0); // Termina o filho corretamente
    }

    return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}