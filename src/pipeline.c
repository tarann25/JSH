#include "pipeline.h"
#include "builtins.h"
#include "error.h"
#include "executor.h"
#include "input.h"
#include "redirect.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void reap_spawned(const pid_t *pids, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (pids[i] > 0) {
            wait_for_child(pids[i], NULL);
        }
    }
}

int execute_pipeline(const Pipeline *pipeline, ShellState *state) {
    size_t n = pipeline->count;
    pid_t *pids = jsh_calloc(n, sizeof(pid_t));
    int prev_read_fd = -1;

    struct sigaction sa_ign, old_int;
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sigaction(SIGINT, &sa_ign, &old_int);

    for (size_t i = 0; i < n; i++) {
        int pipefd[2] = {-1, -1};
        if (i < n - 1) {
            if (pipe(pipefd) == -1) {
                jsh_perror("pipe");
                if (prev_read_fd != -1) {
                    close(prev_read_fd);
                }
                reap_spawned(pids, i);
                sigaction(SIGINT, &old_int, NULL);
                free(pids);
                state->last_status = JSH_ERROR;
                return JSH_ERROR;
            }
            jsh_trace(state->trace_syscalls,
                      "pipe() [stage %zu -> %zu] -> read_fd=%d, write_fd=%d",
                      i, i + 1, pipefd[0], pipefd[1]);
        }

        pid_t pid = fork();
        if (pid == -1) {
            jsh_perror("fork");
            if (prev_read_fd != -1) {
                close(prev_read_fd);
            }
            if (pipefd[0] != -1) {
                close(pipefd[0]);
            }
            if (pipefd[1] != -1) {
                close(pipefd[1]);
            }
            reap_spawned(pids, i);
            sigaction(SIGINT, &old_int, NULL);
            free(pids);
            state->last_status = JSH_ERROR;
            return JSH_ERROR;
        }

        if (pid == 0) {
            free(pids);
            reset_child_signals();

            if (prev_read_fd != -1) {
                jsh_trace(state->trace_syscalls,
                          "stage[%zu] pid=%d dup2(fd=%d -> STDIN_FILENO)",
                          i, (int)getpid(), prev_read_fd);
                if (dup2(prev_read_fd, STDIN_FILENO) == -1) {
                    jsh_perror("dup2(pipe stdin)");
                    _exit(EXIT_FAILURE);
                }
                if (prev_read_fd != STDIN_FILENO) {
                    close(prev_read_fd);
                }
            }

            if (i < n - 1) {
                jsh_trace(state->trace_syscalls,
                          "stage[%zu] pid=%d dup2(fd=%d -> STDOUT_FILENO); close(%d)",
                          i, (int)getpid(), pipefd[1], pipefd[0]);
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    jsh_perror("dup2(pipe stdout)");
                    _exit(EXIT_FAILURE);
                }
                close(pipefd[0]);
                if (pipefd[1] != STDOUT_FILENO) {
                    close(pipefd[1]);
                }
            }

            const Command *cmd = &pipeline->commands[i];
            if (apply_redirections(cmd, state->trace_syscalls) != 0) {
                _exit(EXIT_FAILURE);
            }

            if (is_builtin(cmd->argv[0])) {
                jsh_trace(state->trace_syscalls,
                          "stage[%zu] pid=%d builtin(\"%s\")",
                          i, (int)getpid(), cmd->argv[0]);
                int rc = execute_builtin(cmd, state);
                fflush(stdout);
                _exit(rc);
            }

            jsh_trace(state->trace_syscalls,
                      "stage[%zu] pid=%d execvp(\"%s\")",
                      i, (int)getpid(), cmd->argv[0]);
            execvp(cmd->argv[0], cmd->argv);
            if (errno == ENOENT) {
                jsh_error("%s: command not found", cmd->argv[0]);
                _exit(127);
            }
            jsh_perror(cmd->argv[0]);
            _exit(126);
        }

        pids[i] = pid;
        jsh_trace(state->trace_syscalls,
                  "fork() -> stage[%zu] pid=%d (\"%s\")",
                  i, (int)pid, pipeline->commands[i].argv[0]);
        if (prev_read_fd != -1) {
            close(prev_read_fd);
            prev_read_fd = -1;
        }
        if (i < n - 1) {
            close(pipefd[1]);
            prev_read_fd = pipefd[0];
        }
    }

    bool any_sigint = false;
    for (size_t i = 0; i < n; i++) {
        bool stage_sigint = false;
        int st = wait_for_child(pids[i], &stage_sigint);
        jsh_trace(state->trace_syscalls,
                  "waitpid(%d) [stage %zu] -> status=%d",
                  (int)pids[i], i, st);
        if (stage_sigint) {
            any_sigint = true;
        }
        if (i == n - 1) {
            state->last_status = st;
        }
    }

    sigaction(SIGINT, &old_int, NULL);
    if (any_sigint && isatty(STDIN_FILENO)) {
        putchar('\n');
    }

    free(pids);
    return JSH_SUCCESS;
}
