#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <stddef.h>

#define JSH_SUCCESS 0
#define JSH_ERROR   1

/**
 * Represents a single command and its optional I/O redirections.
 */
typedef struct {
    char **argv;        /* NULL-terminated argument vector for execvp() */
    size_t argc;        /* Number of arguments (excluding trailing NULL) */
    char *input_file;   /* Target file for '<', or NULL */
    char *output_file;  /* Target file for '>' or '>>', or NULL */
    bool append_output; /* true for '>>', false for '>' */
} Command;

/**
 * Represents one or more commands connected by '|' pipelines.
 */
typedef struct {
    Command *commands;  /* Array of pipeline stages */
    size_t count;       /* Number of commands (>= 1 when valid) */
} Pipeline;

/**
 * Persistent state maintained by the interactive shell across iterations.
 */
typedef struct {
    int last_status;    /* Exit code of the most recent foreground command */
    bool should_exit;   /* Set to true by 'exit' built-in or EOF */
    int exit_code;      /* Final exit status returned from main() */
    char *prev_dir;     /* Previous working directory for 'cd -' */
    bool trace_syscalls;/* Live OS syscall/FD tracing enabled via 'trace on' */
} ShellState;

#endif /* SHELL_H */
