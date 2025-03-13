/**
 * @file util.c
 * @brief Implementation of utility functions for HOILC.
 * 
 * This file contains the implementation of utility functions used throughout
 * the compiler.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>

/**
 * @brief Safe implementation of strdup with error checking.
 * 
 * @param str The string to duplicate.
 * @return A newly allocated copy of the string, or NULL on allocation failure.
 */
char* util_strdup(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  
  size_t len = strlen(str);
  char* dup = (char*)malloc(len + 1);
  if (dup == NULL) {
    return NULL;
  }
  
  memcpy(dup, str, len + 1);
  return dup;
}

/**
 * @brief Safe implementation of strndup with error checking.
 * 
 * @param str The string to duplicate.
 * @param max_len The maximum length to copy.
 * @return A newly allocated copy of the string, or NULL on allocation failure.
 */
char* util_strndup(const char* str, size_t max_len) {
  if (str == NULL) {
    return NULL;
  }
  
  size_t len = strlen(str);
  if (len > max_len) {
    len = max_len;
  }
  
  char* dup = (char*)malloc(len + 1);
  if (dup == NULL) {
    return NULL;
  }
  
  memcpy(dup, str, len);
  dup[len] = '\0';
  return dup;
}

/**
 * @brief Safely concatenate two strings.
 * 
 * @param s1 The first string.
 * @param s2 The second string.
 * @return A newly allocated string containing the concatenation, or NULL on failure.
 */
char* util_strcat(const char* s1, const char* s2) {
  if (s1 == NULL || s2 == NULL) {
    return NULL;
  }
  
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  
  char* result = (char*)malloc(len1 + len2 + 1);
  if (result == NULL) {
    return NULL;
  }
  
  memcpy(result, s1, len1);
  memcpy(result + len1, s2, len2 + 1);  /* Include null terminator */
  
  return result;
}

/**
 * @brief Check if a string starts with a prefix.
 * 
 * @param str The string to check.
 * @param prefix The prefix to check for.
 * @return true if the string starts with the prefix, false otherwise.
 */
bool util_starts_with(const char* str, const char* prefix) {
  if (str == NULL || prefix == NULL) {
    return false;
  }
  
  size_t prefix_len = strlen(prefix);
  return strncmp(str, prefix, prefix_len) == 0;
}

/**
 * @brief Check if a string ends with a suffix.
 * 
 * @param str The string to check.
 * @param suffix The suffix to check for.
 * @return true if the string ends with the suffix, false otherwise.
 */
bool util_ends_with(const char* str, const char* suffix) {
  if (str == NULL || suffix == NULL) {
    return false;
  }
  
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  
  if (suffix_len > str_len) {
    return false;
  }
  
  return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/**
 * @brief Convert a string to lowercase.
 * 
 * @param str The string to convert.
 * @return A newly allocated string in lowercase, or NULL on failure.
 */
char* util_to_lower(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  
  size_t len = strlen(str);
  char* result = (char*)malloc(len + 1);
  if (result == NULL) {
    return NULL;
  }
  
  for (size_t i = 0; i < len; i++) {
    result[i] = tolower((unsigned char)str[i]);
  }
  
  result[len] = '\0';
  return result;
}

/**
 * @brief Convert a string to uppercase.
 * 
 * @param str The string to convert.
 * @return A newly allocated string in uppercase, or NULL on failure.
 */
char* util_to_upper(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  
  size_t len = strlen(str);
  char* result = (char*)malloc(len + 1);
  if (result == NULL) {
    return NULL;
  }
  
  for (size_t i = 0; i < len; i++) {
    result[i] = toupper((unsigned char)str[i]);
  }
  
  result[len] = '\0';
  return result;
}

/**
 * @brief Safe memory allocation with error handling.
 * 
 * @param size The size to allocate.
 * @return The allocated memory or NULL on failure.
 */
void* util_malloc(size_t size) {
  void* ptr = malloc(size);
  if (ptr == NULL && size > 0) {
    /* Handle allocation failure */
    fprintf(stderr, "Error: Memory allocation failed for size %zu\n", size);
  }
  
  return ptr;
}

/**
 * @brief Safe memory reallocation with error handling.
 * 
 * @param ptr The pointer to reallocate.
 * @param size The new size.
 * @return The reallocated memory or NULL on failure.
 */
void* util_realloc(void* ptr, size_t size) {
  void* new_ptr = realloc(ptr, size);
  if (new_ptr == NULL && size > 0) {
    /* Handle allocation failure */
    fprintf(stderr, "Error: Memory reallocation failed for size %zu\n", size);
  }
  
  return new_ptr;
}

/**
 * @brief Safe zeroed memory allocation with error handling.
 * 
 * @param nmemb Number of elements.
 * @param size Element size.
 * @return The allocated memory or NULL on failure.
 */
void* util_calloc(size_t nmemb, size_t size) {
  void* ptr = calloc(nmemb, size);
  if (ptr == NULL && nmemb > 0 && size > 0) {
    /* Handle allocation failure */
    fprintf(stderr, "Error: Memory allocation failed for size %zu x %zu\n", 
            nmemb, size);
  }
  
  return ptr;
}

/**
 * @brief Safe aligned memory allocation with error handling.
 * 
 * @param alignment The alignment boundary.
 * @param size The size to allocate.
 * @return The aligned allocated memory or NULL on failure.
 */
void* util_aligned_alloc(size_t alignment, size_t size) {
  /* Adjust size to be a multiple of alignment */
  size_t adjusted_size = (size + alignment - 1) & ~(alignment - 1);
  
  void* ptr;
#if defined(_WIN32)
  ptr = _aligned_malloc(adjusted_size, alignment);
#else
  /* posix_memalign requires size to be a multiple of alignment */
  if (posix_memalign(&ptr, alignment, adjusted_size) != 0) {
    ptr = NULL;
  }
#endif
  
  if (ptr == NULL && size > 0) {
    /* Handle allocation failure */
    fprintf(stderr, "Error: Aligned memory allocation failed for size %zu, alignment %zu\n", 
            size, alignment);
  }
  
  return ptr;
}

/**
 * @brief Get the basename of a file path.
 * 
 * @param path The file path.
 * @return The basename (without directory).
 */
const char* util_basename(const char* path) {
  if (path == NULL) {
    return NULL;
  }
  
  const char* last_slash = strrchr(path, '/');
#ifdef _WIN32
  const char* last_backslash = strrchr(path, '\\');
  if (last_backslash != NULL && (last_slash == NULL || last_backslash > last_slash)) {
    last_slash = last_backslash;
  }
#endif
  
  if (last_slash != NULL) {
    return last_slash + 1;
  }
  
  return path;
}

/**
 * @brief Get the directory part of a file path.
 * 
 * @param path The file path.
 * @return A newly allocated string containing the directory part, or NULL on failure.
 */
char* util_dirname(const char* path) {
  if (path == NULL) {
    return NULL;
  }
  
  const char* last_slash = strrchr(path, '/');
#ifdef _WIN32
  const char* last_backslash = strrchr(path, '\\');
  if (last_backslash != NULL && (last_slash == NULL || last_backslash > last_slash)) {
    last_slash = last_backslash;
  }
#endif
  
  if (last_slash == NULL) {
    /* No directory part */
    return util_strdup(".");
  }
  
  /* Get the directory part */
  size_t dir_len = last_slash - path;
  if (dir_len == 0) {
    /* Root directory */
    return util_strdup("/");
  }
  
  return util_strndup(path, dir_len);
}

/**
 * @brief Check if a file exists.
 * 
 * @param filename The file path to check.
 * @return true if the file exists, false otherwise.
 */
bool util_file_exists(const char* filename) {
  if (filename == NULL) {
    return false;
  }
  
  struct stat buffer;
  return stat(filename, &buffer) == 0;
}

/**
 * @brief Read an entire file into memory.
 * 
 * @param filename The file to read.
 * @param content Pointer to store the file content.
 * @param size Pointer to store the file size.
 * @return true on success, false on failure.
 */
bool util_read_file(const char* filename, char** content, size_t* size) {
  assert(filename != NULL);
  assert(content != NULL);
  assert(size != NULL);
  
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    return false;
  }
  
  /* Get file size */
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return false;
  }
  
  long file_size = ftell(file);
  if (file_size < 0) {
    fclose(file);
    return false;
  }
  
  if (fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return false;
  }
  
  /* Allocate buffer for file content */
  *content = (char*)malloc(file_size + 1);
  if (*content == NULL) {
    fclose(file);
    return false;
  }
  
  /* Read file content */
  size_t bytes_read = fread(*content, 1, file_size, file);
  if (bytes_read != (size_t)file_size) {
    free(*content);
    fclose(file);
    return false;
  }
  
  /* Null-terminate the content */
  (*content)[file_size] = '\0';
  
  *size = (size_t)file_size;
  
  fclose(file);
  return true;
}

/**
 * @brief Write binary data to a file.
 * 
 * @param filename The file to write.
 * @param data The data to write.
 * @param size The size of the data.
 * @return true on success, false on failure.
 */
bool util_write_file(const char* filename, const void* data, size_t size) {
  assert(filename != NULL);
  assert(data != NULL || size == 0);
  
  FILE* file = fopen(filename, "wb");
  if (file == NULL) {
    return false;
  }
  
  /* Write data */
  size_t bytes_written = fwrite(data, 1, size, file);
  if (bytes_written != size) {
    fclose(file);
    return false;
  }
  
  fclose(file);
  return true;
}

/**
 * @brief Compute the current timestamp.
 * 
 * @return The current timestamp in milliseconds.
 */
uint64_t util_timestamp(void) {
  struct timespec ts;
  
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    return 0;
  }
  
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/**
 * @brief Format a human-readable size string.
 * 
 * @param size The size in bytes.
 * @param buffer The buffer to store the result.
 * @param buffer_size The size of the buffer.
 * @return The buffer.
 */
char* util_format_size(size_t size, char* buffer, size_t buffer_size) {
  assert(buffer != NULL);
  assert(buffer_size > 0);
  
  const char* units[] = {"B", "KB", "MB", "GB", "TB"};
  size_t unit_count = sizeof(units) / sizeof(units[0]);
  
  size_t unit_index = 0;
  double size_value = (double)size;
  
  /* Convert to appropriate unit */
  while (size_value >= 1024.0 && unit_index < unit_count - 1) {
    size_value /= 1024.0;
    unit_index++;
  }
  
  /* Format the result */
  if (unit_index == 0) {
    /* Bytes, no decimal points */
    snprintf(buffer, buffer_size, "%zu %s", (size_t)size_value, units[unit_index]);
  } else {
    /* Larger units, with 2 decimal points */
    snprintf(buffer, buffer_size, "%.2f %s", size_value, units[unit_index]);
  }
  
  return buffer;
}

/**
 * @brief Format a human-readable time string.
 * 
 * @param time_ms The time in milliseconds.
 * @param buffer The buffer to store the result.
 * @param buffer_size The size of the buffer.
 * @return The buffer.
 */
char* util_format_time(uint64_t time_ms, char* buffer, size_t buffer_size) {
  assert(buffer != NULL);
  assert(buffer_size > 0);
  
  if (time_ms < 1000) {
    /* Milliseconds */
    snprintf(buffer, buffer_size, "%llu ms", (unsigned long long)time_ms);
  } else if (time_ms < 60000) {
    /* Seconds with 2 decimal points */
    double seconds = (double)time_ms / 1000.0;
    snprintf(buffer, buffer_size, "%.2f s", seconds);
  } else if (time_ms < 3600000) {
    /* Minutes and seconds */
    uint64_t minutes = time_ms / 60000;
    uint64_t seconds = (time_ms % 60000) / 1000;
    snprintf(buffer, buffer_size, "%llu m %llu s", 
            (unsigned long long)minutes, (unsigned long long)seconds);
  } else {
    /* Hours, minutes, and seconds */
    uint64_t hours = time_ms / 3600000;
    uint64_t minutes = (time_ms % 3600000) / 60000;
    uint64_t seconds = (time_ms % 60000) / 1000;
    snprintf(buffer, buffer_size, "%llu h %llu m %llu s", 
            (unsigned long long)hours, (unsigned long long)minutes, 
            (unsigned long long)seconds);
  }
  
  return buffer;
}