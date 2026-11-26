#ifndef PIPELINE_H
#define PIPELINE_H

#include "shell.h"

/**
 * Execute a pipeline of 2 or more commands connected via POSIX pipe(),
 * fork(), dup2(), and waitpid(). Ensures all unused descriptors are closed
 * and all child processes are reaped.
 */
int execute_pipeline(const Pipeline *pipeline, ShellState *state);

#endif /* PIPELINE_H */
