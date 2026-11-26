#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell.h"

#include <sys/types.h>

/**
 * Execute a validated Pipeline (either a single command or a multi-stage pipeline).
 * Updates state->last_status and returns 0 on dispatch success.
 */
int execute_pipeline_or_command(const Pipeline *pipeline, ShellState *state);

/**
 * Reap a single child process via waitpid() (retrying on EINTR) and convert its
 * termination status to a standard shell exit code.
 */
int wait_for_child(pid_t pid, bool *interrupted_by_sigint);

#endif /* EXECUTOR_H */
