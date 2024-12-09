#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h> 
#include <string.h>
#include <errno.h>

#include "constants.h"
#include "parser.h"
#include "fileManipulation.h"
#include "operations.h"

int is_job_file(const char *filename) {
    
    const char *ext = strrchr(filename, '.');

    
    if (ext == NULL || strcmp(ext, ".job") != 0) {
        return 0;
    }

    return 1;
}

int process_file(const char* file_path) {

    int fd = open(file_path,O_RDONLY); 
    printf("Opening file: %s\n", file_path);

    if (fd < 0) {
        printf("Error number: %d\n", errno);
        perror("Error opening file\n");
    }

    char* out_file_path = get_output_filename(file_path);

    int fd_out = open(out_file_path,O_WRONLY | O_CREAT | O_TRUNC, 0644);
    printf("Opening output file: %s\n", out_file_path);

    if (fd_out < 0) {
        printf("Error number: %d\n", errno);
        perror("Error opening file\n");
    }


    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    unsigned int delay;
    size_t num_pairs;
    char output[MAX_WRITE_SIZE];

    // int com_counter = 0;

    while (1) {
        
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
                writeInFile(output, fd_out);

                break;

            case CMD_DELETE:

                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                
                }

                if (kvs_delete(num_pairs, keys, output)) {
                    fprintf(stderr, "Failed to delete pair\n");
                }
                writeInFile(output, fd_out);
                break;

            case CMD_SHOW:

                kvs_show(output);
                writeInFile(output, fd_out);
                break;

            case CMD_WAIT:
                if (parse_wait(fd, &delay, NULL) == -1) {
                    fprintf(stderr, "Invalid command. See HELP for usage\n");
                
                }

                if (delay > 0) {
                    printf("Waiting...\n");
                    kvs_wait(delay);
                }
                break;

            case CMD_BACKUP:

                if (kvs_backup()) {
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
                    "  BACKUP\n" // Not implemented
                    "  HELP\n"
                );

                break;
                
            case CMD_EMPTY:
                break;

            case EOC:
                // kvs_terminate();
                close(fd);
                return 0;
        }
    }

}

char* get_output_filename(const char *input_filename) {
    // Verifica se o arquivo tem a extensão .job
    const char *ext = strrchr(input_filename, '.'); // Encontra o último ponto (.)
    
    if (ext != NULL && strcmp(ext, ".job") == 0) {
        // Cria um buffer para armazenar o novo nome do arquivo
        static char output_filename[MAX_WRITE_SIZE];
        
        // Copia o nome do arquivo sem a extensão ".job"
        size_t length = (size_t)(ext - input_filename);
        strncpy(output_filename, input_filename, length);
        
        // Adiciona a nova extensão ".out"
        strcpy(output_filename + (ext - input_filename), ".out");
        
        return output_filename;
    } else {
        // Se não for um arquivo .job, retorna NULL ou um nome default
        return NULL;
    }
}

int writeInFile(char output[MAX_WRITE_SIZE], int fd) {


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

int readFiles(char* path) {

    struct dirent *file;
    DIR *dir = opendir(path);
    //int file_count = 0;

    if (dir == NULL) {
        perror("Error Opening Directory\n");
        return -1; 
    }

    size_t path_len = strlen(path);
    char aux_path[path_len + MAX_PATH];
    // Itera sobre todos os arquivos no diretório
    while ((file = readdir(dir)) != NULL) {
        
        // Verifica se o arquivo corresponde ao formato esperado
        if (is_job_file(file->d_name)) {

            printf("fileName: %s\n", file->d_name);

            snprintf(aux_path, sizeof(aux_path), "%s/%s", path, file->d_name);

            printf("filePath: %s\n", aux_path);

            process_file(aux_path);  // Processa o arquivo
        }

        
    }

    kvs_terminate();

    closedir(dir);  // Fecha o diretório após a leitura
    return 0;
}