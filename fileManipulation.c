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


int write_in_file(char output[MAX_WRITE_SIZE], int fd) {


    printf("outputToWrite: %s\n", output);
    
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
