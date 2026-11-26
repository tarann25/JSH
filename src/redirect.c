#include "redirect.h"
#include "error.h"

#include <fcntl.h>
#include <unistd.h>

int apply_redirections(const Command *cmd, bool trace) {
    if (cmd->input_file != NULL) {
        int in_fd = open(cmd->input_file, O_RDONLY);
        if (in_fd == -1) {
            jsh_perror(cmd->input_file);
            return -1;
        }
        jsh_trace(trace,
                  "pid=%d open(\"%s\", O_RDONLY) -> fd=%d; dup2(%d, STDIN_FILENO)",
                  (int)getpid(), cmd->input_file, in_fd, in_fd);
        if (dup2(in_fd, STDIN_FILENO) == -1) {
            jsh_perror("dup2(stdin)");
            close(in_fd);
            return -1;
        }
        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
    }

    if (cmd->output_file != NULL) {
        int flags = O_WRONLY | O_CREAT | (cmd->append_output ? O_APPEND : O_TRUNC);
        int out_fd = open(cmd->output_file, flags, 0644);
        if (out_fd == -1) {
            jsh_perror(cmd->output_file);
            return -1;
        }
        jsh_trace(trace,
                  "pid=%d open(\"%s\", %s) -> fd=%d; dup2(%d, STDOUT_FILENO)",
                  (int)getpid(), cmd->output_file,
                  cmd->append_output ? "O_WRONLY|O_CREAT|O_APPEND" : "O_WRONLY|O_CREAT|O_TRUNC",
                  out_fd, out_fd);
        if (dup2(out_fd, STDOUT_FILENO) == -1) {
            jsh_perror("dup2(stdout)");
            close(out_fd);
            return -1;
        }
        if (out_fd != STDOUT_FILENO) {
            close(out_fd);
        }
    }

    return 0;
}

int save_stdio(int saved_fds[2]) {
    saved_fds[0] = dup(STDIN_FILENO);
    if (saved_fds[0] == -1) {
        jsh_perror("dup(stdin)");
        return -1;
    }

    saved_fds[1] = dup(STDOUT_FILENO);
    if (saved_fds[1] == -1) {
        jsh_perror("dup(stdout)");
        close(saved_fds[0]);
        saved_fds[0] = -1;
        return -1;
    }

    return 0;
}

int restore_stdio(int saved_fds[2]) {
    int rc = 0;

    if (saved_fds[0] != -1) {
        if (dup2(saved_fds[0], STDIN_FILENO) == -1) {
            jsh_perror("dup2(restore stdin)");
            rc = -1;
        }
        close(saved_fds[0]);
        saved_fds[0] = -1;
    }

    if (saved_fds[1] != -1) {
        if (dup2(saved_fds[1], STDOUT_FILENO) == -1) {
            jsh_perror("dup2(restore stdout)");
            rc = -1;
        }
        close(saved_fds[1]);
        saved_fds[1] = -1;
    }

    return rc;
}
