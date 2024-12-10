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
#include "fileManipulation.h"
#include "operations.h"



int is_job_file(const char *filename) {
    
    const char *ext = strrchr(filename, '.');
    //printf("Extension: %s\n", ext);
    
    if (ext == NULL || strcmp(ext, ".job") != 0) {
        return 0;
    }

    return 1;
}


/**
 * Modifica o caminho do arquivo, alterando, adicionando ou removendo extensões.
 * 
 * - Para remover a extensão: passe `new_ext` como uma string vazia ("").
 * - Para adicionar uma nova extensão: passe `old_ext` como NULL.
 * - Para substituir a extensão: forneça tanto `old_ext` quanto `new_ext`.
 * 
 * @param file_path Caminho original do arquivo.
 * @param old_ext Extensão existente esperada (ou NULL para ignorar).
 * @param new_ext Nova extensão desejada (ou "" para remover).
 * @return Caminho alterado ou NULL em caso de erro.
 */
char *modify_file_path(const char *file_path, const char *old_ext, const char *new_ext) {
    
    const char *ext = NULL;
    if(old_ext != NULL){
        ext = strrchr(file_path, '.');
    } // Encontra o último ponto (.)

    size_t file_path_length = strlen(file_path);

    // Buffer para a nova string (aloca espaço suficiente)
    size_t new_length = file_path_length + (new_ext ? strlen(new_ext) : 0) + 1;
    char *result = malloc(new_length);
    if (!result) {
        return NULL; // Falha ao alocar memória
    }

    // Verifica se deve remover ou substituir uma extensão existente
    if (ext != NULL && (old_ext == NULL || strcmp(ext, old_ext) == 0)) {
        size_t length = (size_t)(ext - file_path); // Calcula o comprimento antes da extensão
        strncpy(result, file_path, length);       // Copia o nome sem a extensão antiga
        result[length] = '\0';                   // Garante terminação da string

        // Adiciona a nova extensão, se fornecida
        if (new_ext && new_ext[0] != '\0') {
            strcat(result, new_ext);
        }
        return result; // Sucesso
    }

    // Caso não seja necessário alterar ou adicionar uma extensão nova
    if (old_ext == NULL && new_ext && new_ext[0] != '\0') {
        snprintf(result, new_length, "%s%s", file_path, new_ext);
        return result; // Sucesso
    }

    // Caso contrário, copia o nome original sem alterações
    strncpy(result, file_path, new_length);

    result[new_length - 1] = '\0'; // Garante terminação
    return result;
}

/**
 * Remove uma extensão específica de um caminho de arquivo.
 */
char *remove_extension(const char *file_path, const char *ext) {
    return modify_file_path(file_path, ext, "");
}

/**
 * Adiciona uma extensão específica a um caminho de arquivo.
 */
char *add_extension(const char *file_path, const char *ext) {
    return modify_file_path(file_path, NULL, ext);
}


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
                num_pairs = parse_write(fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);

                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                
                }

                if (kvs_write(num_pairs, keys, values)) {
                    fprintf(stderr, "Failed to write pair\n");
                }

                break;

            case CMD_READ:
                
                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
                
                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                    
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
                
                }

                if (delay > 0) {
                    strcpy(output, "Waiting...\n");
                    write_in_file(output, fd_out);
                    kvs_wait(delay);
                }
                break;

            case CMD_BACKUP:

                backupCounter++;
                // printf("BackupCounter: %d\n", backupCounter);

                if (kvs_backup(file_path, backupCounter)) { // Passa o caminho para `kvs_backup`
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
                // kvs_terminate();
                wait(NULL);
                free(out_file_path);
                close(fd);
                close(fd_out);
                return 0;
        }
    }

}

int write_in_file(char output[MAX_WRITE_SIZE], int fd) {


    // printf("outputToWrite: %s\n", output);
    
    ssize_t bytes_written = 0;
    ssize_t total_written = 0;

    
    while (output[total_written] != '\0') { 
        bytes_written = write(fd, &output[total_written], 1); 

        if (bytes_written < 0) {
            printf("Error number: %d\n", errno);
            perror("Error writing to file");
            return 0;
        }

        total_written += bytes_written; // Atualiza a quantidade de bytes escritos
    }

    return 0;
}
