#ifndef REDIRECT_H
#define REDIRECT_H

#include "shell.h"

/**
 * Apply input (<) and output (> / >>) redirections for cmd using open() and dup2().
 * Returns 0 on success, -1 on failure (error message printed, opened FDs closed).
 */
int apply_redirections(const Command *cmd, bool trace);

/** Save STDIN_FILENO and STDOUT_FILENO into saved_fds[2]. Returns 0 or -1. */
int save_stdio(int saved_fds[2]);

/** Restore STDIN_FILENO and STDOUT_FILENO from saved_fds[2] and close duplicates. */
int restore_stdio(int saved_fds[2]);

#endif /* REDIRECT_H */
