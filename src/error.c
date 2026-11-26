#include "error.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void jsh_error(const char *fmt, ...) {
    va_list args;
    fputs("jsh: ", stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

void jsh_perror(const char *context) {
    int err = errno;
    if (context != NULL && context[0] != '\0') {
        fprintf(stderr, "jsh: %s: %s\n", context, strerror(err));
    } else {
        fprintf(stderr, "jsh: %s\n", strerror(err));
    }
}

void jsh_trace(bool enabled, const char *fmt, ...) {
    if (!enabled) {
        return;
    }

    bool use_color = isatty(STDERR_FILENO) != 0;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (use_color) {
        fprintf(stderr, "\033[0;36m[trace]\033[0m %s\n", buf);
    } else {
        fprintf(stderr, "[trace] %s\n", buf);
    }
    fflush(stderr);
}

void *jsh_malloc(size_t size) {
    void *ptr = malloc(size == 0 ? 1 : size);
    if (ptr == NULL) {
        jsh_perror("malloc");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *jsh_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb == 0 ? 1 : nmemb, size == 0 ? 1 : size);
    if (ptr == NULL) {
        jsh_perror("calloc");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *jsh_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size == 0 ? 1 : size);
    if (new_ptr == NULL) {
        jsh_perror("realloc");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

char *jsh_strdup(const char *s) {
    size_t len = strlen(s);
    char *copy = jsh_malloc(len + 1);
    memcpy(copy, s, len + 1);
    return copy;
}

char *jsh_strndup(const char *s, size_t n) {
    size_t len = 0;
    while (len < n && s[len] != '\0') {
        len++;
    }
    char *copy = jsh_malloc(len + 1);
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}
