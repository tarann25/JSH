#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include <stddef.h>

/** Print a formatted error message prefixed with "jsh: " to stderr. */
void jsh_error(const char *fmt, ...);

/** Print a system error message prefixed with "jsh: <context>: " using errno. */
void jsh_perror(const char *context);

/** Print a live syscall/FD trace message to stderr when tracing is enabled. */
void jsh_trace(bool enabled, const char *fmt, ...);

/** Checked malloc wrapper; terminates on unrecoverable OOM. */
void *jsh_malloc(size_t size);

/** Checked calloc wrapper; terminates on unrecoverable OOM. */
void *jsh_calloc(size_t nmemb, size_t size);

/** Checked realloc wrapper; terminates on unrecoverable OOM. */
void *jsh_realloc(void *ptr, size_t size);

/** Checked strdup wrapper; terminates on unrecoverable OOM. */
char *jsh_strdup(const char *s);

/** Checked strndup wrapper; terminates on unrecoverable OOM. */
char *jsh_strndup(const char *s, size_t n);

#endif /* ERROR_H */
