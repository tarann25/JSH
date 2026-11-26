#include "executor.h"
#include "builtins.h"
#include "error.h"
#include "input.h"
#include "pipeline.h"
#include "redirect.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int wait_for_child(pid_t pid, bool *interrupted_by_sigint) {
    int status = 0;
    pid_t res;

    if (interrupted_by_sigint != NULL) {
        *interrupted_by_sigint = false;
    }

    do {
        res = waitpid(pid, &status, 0);
    } while (res == -1 && errno == EINTR);

    if (res == -1) {
        jsh_perror("waitpid");
        return JSH_ERROR;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGINT && interrupted_by_sigint != NULL) {
            *interrupted_by_sigint = true;
        }
        return 128 + sig;
    }

    return JSH_ERROR;
}

static int execute_single_command(const Command *cmd, ShellState *state) {
    if (is_builtin(cmd->argv[0])) {
        bool has_redir = (cmd->input_file != NULL || cmd->output_file != NULL);
        int saved_fds[2] = {-1, -1};

        if (has_redir) {
            if (save_stdio(saved_fds) != 0) {
                state->last_status = JSH_ERROR;
                return JSH_ERROR;
            }
            if (apply_redirections(cmd, state->trace_syscalls) != 0) {
                restore_stdio(saved_fds);
                state->last_status = JSH_ERROR;
                return JSH_ERROR;
            }
        }

        jsh_trace(state->trace_syscalls,
                  "builtin \"%s\" executing in parent pid=%d (no fork)",
                  cmd->argv[0], (int)getpid());
        int rc = execute_builtin(cmd, state);
        fflush(stdout);

        if (has_redir) {
            restore_stdio(saved_fds);
        }
        return rc;
    }

    /* Ignore SIGINT in parent while foreground child executes */
    struct sigaction sa_ign, old_int;
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sigaction(SIGINT, &sa_ign, &old_int);

    pid_t pid = fork();
    if (pid == -1) {
        jsh_perror("fork");
        sigaction(SIGINT, &old_int, NULL);
        state->last_status = JSH_ERROR;
        return JSH_ERROR;
    }

    if (pid == 0) {
        reset_child_signals();

        if (apply_redirections(cmd, state->trace_syscalls) != 0) {
            _exit(EXIT_FAILURE);
        }

        jsh_trace(state->trace_syscalls,
                  "child pid=%d execvp(\"%s\")",
                  (int)getpid(), cmd->argv[0]);
        execvp(cmd->argv[0], cmd->argv);
        if (errno == ENOENT) {
            jsh_error("%s: command not found", cmd->argv[0]);
            _exit(127);
        }
        jsh_perror(cmd->argv[0]);
        _exit(126);
    }

    jsh_trace(state->trace_syscalls,
              "parent pid=%d fork() -> child pid=%d (\"%s\")",
              (int)getpid(), (int)pid, cmd->argv[0]);

    bool sigint_child = false;
    state->last_status = wait_for_child(pid, &sigint_child);
    jsh_trace(state->trace_syscalls,
              "waitpid(%d) -> status=%d",
              (int)pid, state->last_status);
    sigaction(SIGINT, &old_int, NULL);

    if (sigint_child && isatty(STDIN_FILENO)) {
        putchar('\n');
    }

    return JSH_SUCCESS;
}

int execute_pipeline_or_command(const Pipeline *pipeline, ShellState *state) {
    if (pipeline == NULL || pipeline->count == 0) {
        return JSH_SUCCESS;
    }

    if (pipeline->count == 1) {
        return execute_single_command(&pipeline->commands[0], state);
    }

    return execute_pipeline(pipeline, state);
}
