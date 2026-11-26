#ifndef INPUT_H
#define INPUT_H

#include "shell.h"

typedef enum {
    INPUT_OK = 0,          /* Non-empty line read into *line_out */
    INPUT_EMPTY,           /* Blank / whitespace-only line */
    INPUT_INTERRUPTED,     /* Interrupted by SIGINT (Ctrl+C) */
    INPUT_EOF              /* End of input (Ctrl+D) or stream closed */
} InputResult;

/** Install interactive SIGINT and SIGQUIT handlers for the shell process. */
void install_interactive_signals(void);

/** Reset signal handlers to default before execvp() in a child process. */
void reset_child_signals(void);

/** Display the shell prompt and read one line using getline(). */
InputResult read_command_line(char **line_out, size_t *cap_out, const ShellState *state);

#endif /* INPUT_H */
