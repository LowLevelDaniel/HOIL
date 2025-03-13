/**
 * @file util.h
 * @brief Utility functions for HOILC.
 * 
 * This header defines utility functions used throughout the compiler.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_UTIL_H
#define HOILC_UTIL_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Check if a file exists.
 * 
 * @param filename The file path to check.
 * @return true if the file exists, false otherwise.
 */
bool util_file_exists(const char* filename);

/**
 * @brief Read an entire file into memory.
 * 
 * @param filename The file to read.
 * @param content Pointer to store the file content.
 * @param size Pointer to store the file size.
 * @return true on success, false on failure.
 */
bool util_read_file(const char* filename, char** content, size_t* size);

/**
 * @brief Write binary data to a file.
 * 
 * @param filename The file to write.
 * @param data The data to write.
 * @param size The size of the data.
 * @return true on success, false on failure.
 */
bool util_write_file(const char* filename, const void* data, size_t size);

/**
 * @brief Safely duplicate a string.
 * 
 * @param str The string to duplicate.
 * @return A newly allocated copy of the string, or NULL on allocation failure.
 */
char* util_strdup(const char* str);

/**
 * @brief Safely duplicate a string with a maximum length.
 * 
 * @param str The string to duplicate.
 * @param max_len The maximum length to copy.
 * @return A newly allocated copy of the string, or NULL on allocation failure.
 */
char* util_strndup(const char* str, size_t max_len);

/**
 * @brief Safely concatenate two strings.
 * 
 * @param s1 The first string.
 * @param s2 The second string.
 * @return A newly allocated string containing the concatenation, or NULL on failure.
 */
char* util_strcat(const char* s1, const char* s2);

/**
 * @brief Check if a string starts with a prefix.
 * 
 * @param str The string to check.
 * @param prefix The prefix to check for.
 * @return true if the string starts with the prefix, false otherwise.
 */
bool util_starts_with(const char* str, const char* prefix);

/**
 * @brief Check if a string ends with a suffix.
 * 
 * @param str The string to check.
 * @param suffix The suffix to check for.
 * @return true if the string ends with the suffix, false otherwise.
 */
bool util_ends_with(const char* str, const char* suffix);

/**
 * @brief Convert a string to lowercase.
 * 
 * @param str The string to convert.
 * @return A newly allocated string in lowercase, or NULL on failure.
 */
char* util_to_lower(const char* str);

/**
 * @brief Convert a string to uppercase.
 * 
 * @param str The string to convert.
 * @return A newly allocated string in uppercase, or NULL on failure.
 */
char* util_to_upper(const char* str);

/**
 * @brief Safe memory allocation with error handling.
 * 
 * @param size The size to allocate.
 * @return The allocated memory or NULL on failure.
 */
void* util_malloc(size_t size);

/**
 * @brief Safe memory reallocation with error handling.
 * 
 * @param ptr The pointer to reallocate.
 * @param size The new size.
 * @return The reallocated memory or NULL on failure.
 */
void* util_realloc(void* ptr, size_t size);

/**
 * @brief Safe zeroed memory allocation with error handling.
 * 
 * @param size The size to allocate.
 * @return The allocated memory or NULL on failure.
 */
void* util_calloc(size_t nmemb, size_t size);

/**
 * @brief Safe aligned memory allocation with error handling.
 * 
 * @param alignment The alignment boundary.
 * @param size The size to allocate.
 * @return The aligned allocated memory or NULL on failure.
 */
void* util_aligned_alloc(size_t alignment, size_t size);

/**
 * @brief Get the basename of a file path.
 * 
 * @param path The file path.
 * @return The basename (without directory).
 */
const char* util_basename(const char* path);

/**
 * @brief Get the directory part of a file path.
 * 
 * @param path The file path.
 * @return A newly allocated string containing the directory part, or NULL on failure.
 */
char* util_dirname(const char* path);

/**
 * @brief Compute the current timestamp.
 * 
 * @return The current timestamp in milliseconds.
 */
uint64_t util_timestamp(void);

/**
 * @brief Format a human-readable size string.
 * 
 * @param size The size in bytes.
 * @param buffer The buffer to store the result.
 * @param buffer_size The size of the buffer.
 * @return The buffer.
 */
char* util_format_size(size_t size, char* buffer, size_t buffer_size);

/**
 * @brief Format a human-readable time string.
 * 
 * @param time_ms The time in milliseconds.
 * @param buffer The buffer to store the result.
 * @param buffer_size The size of the buffer.
 * @return The buffer.
 */
char* util_format_time(uint64_t time_ms, char* buffer, size_t buffer_size);

#endif /* HOILC_UTIL_H */