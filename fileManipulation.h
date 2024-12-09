int readFiles(char* dir_path);
int is_job_file(const char *filename);
int process_file(const char* filename);
char* get_output_filename(const char *input_filename);
int writeInFile(char output[MAX_WRITE_SIZE], const char *filename);
