#ifndef KVS_OPERATIONS_H
#define KVS_OPERATIONS_H

#include <stddef.h>

/// Initializes the KVS state.
/// @return 0 if the KVS state was initialized successfully, 1 otherwise.
int kvs_init();

/// Destroys the KVS state.
/// @return 0 if the KVS state was terminated successfully, 1 otherwise.
int kvs_terminate();

/// Writes a key value pair to the KVS. If key already exists it is updated.
/// @param num_pairs Number of pairs being written.
/// @param keys Array of keys' strings.
/// @param values Array of values' strings.
/// @return 0 if the pairs were written successfully, 1 otherwise.
int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]);

/// Reads values from the KVS.
/// @param num_pairs Number of pairs to read.
/// @param keys Array of keys' strings.
/// @param fd File descriptor to write the (successful) output.
/// @return 0 if the key reading, 1 otherwise.
int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], char output[MAX_WRITE_SIZE]);

/// Deletes key value pairs from the KVS.
/// @param num_pairs Number of pairs to read.
/// @param keys Array of keys' strings.
/// @return 0 if the pairs were deleted successfully, 1 otherwise.
int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], char output[MAX_WRITE_SIZE]);

/// Writes the state of the KVS.
/// @param fd File descriptor to write the output.
void kvs_show(char output[MAX_WRITE_SIZE]);

/// Creates a backup of the KVS state and stores it in the correspondent
/// backup file
/// @return 0 if the backup was successful, 1 otherwise.
int kvs_backup(const char* backup_file_path, int backupCounter, int maxBackups);

/// Waits for a given amount of time.
/// @param delay_us Delay in milliseconds.
void kvs_wait(unsigned int delay_ms);

/// Checks if a file has the ".job" extension.
/// @param filename Name of the file to check.
/// @return 1 if the file has the ".job" extension, 0 otherwise.
int is_job_file(const char *filename);

/// Modifies the extension of a given file path.
/// @param file_path Original file path.
/// @param old_ext Extension to be replaced.
/// @param new_ext New extension to be added.
/// @return Pointer to the new file path with the modified extension.
char *modify_file_path(const char *file_path, const char *old_ext, const char *new_ext);

/// Removes a specific extension from a given file path.
/// @param file_path Original file path.
/// @param ext Extension to be removed.
/// @return Pointer to the file path without the specified extension.
char *remove_extension(const char *file_path, const char *ext);

/// Adds a specific extension to a given file path.
/// @param file_path Original file path.
/// @param ext Extension to be added.
/// @return Pointer to the new file path with the added extension.
char *add_extension(const char *file_path, const char *ext);

/// Writes the given output to a specified file descriptor.
/// @param output The string to be written.
/// @param fd File descriptor where the string will be written.
/// @return 0 if the string was written successfully, -1 otherwise.
int write_in_file(char output[MAX_WRITE_SIZE], int fd);

#endif  // KVS_OPERATIONS_H

