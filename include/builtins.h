#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

/** Return true if cmd_name is handled internally by jsh. */
bool is_builtin(const char *cmd_name);

/**
 * Execute a built-in command in the current process and update state->last_status.
 * Returns the command exit status (0 on success, non-zero on error).
 */
int execute_builtin(const Command *cmd, ShellState *state);

#endif /* BUILTINS_H */
