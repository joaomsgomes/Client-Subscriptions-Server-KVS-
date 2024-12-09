int readFiles(char* dir_path);
int is_job_file(const char *filename);
int process_file(const char* filename);
char* get_filename(const char *input_filename, char extension[EXTENSION]);
int writeInFile(char output[MAX_WRITE_SIZE], int fd);
void remove_job_extension(const char *file_path, char *output_path);
