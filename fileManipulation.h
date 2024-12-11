
int is_job_file(const char *filename);
//int process_file(const char* filename);
int write_in_file(char output[MAX_WRITE_SIZE], int fd);
char *modify_file_path(const char *file_path, const char *old_ext, const char *new_ext);
char *remove_extension(const char *file_path, const char *ext);
char *add_extension(const char *file_path, const char *ext);
// void wait_for_backup_slot(int backupCounter, int maxBackups);