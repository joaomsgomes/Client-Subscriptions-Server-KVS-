#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include "fileManipulation.h"

int is_job_file(const char *filename) {
    // Localiza o último ponto (.) no nome do ficheiro
    const char *ext = strrchr(filename, '.');

    // Se não houver ponto ou a extensão não for ".job", retorna 0 (falso)
    if (ext == NULL || strcmp(ext, ".job") != 0) {
        return 0;  // Não é um ficheiro .job
    }

    return 1;  // É um ficheiro .job
}

void readFiles(char* dir_path) {

    struct dirent *file;
    DIR *dir = opendir(dir_path);
    int file_count = 0;

    if (dir == NULL) {
        perror("Erro ao abrir diretoria");
        return -1; 
    }

    while((file = readdir) != NULL) {
        
        if (is_job_file(file)) {
            execute();
        }
        

    }


}