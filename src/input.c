#include "input.h"
#include "error.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static volatile sig_atomic_t g_sigint_received = 0;

static void sigint_handler(int signo) {
    (void)signo;
    g_sigint_received = 1;
}

void install_interactive_signals(void) {
    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; /* Interrupt blocking getline() without SA_RESTART */
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        jsh_perror("sigaction(SIGINT)");
    }

    struct sigaction sa_quit;
    memset(&sa_quit, 0, sizeof(sa_quit));
    sa_quit.sa_handler = SIG_IGN;
    sigemptyset(&sa_quit.sa_mask);
    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        jsh_perror("sigaction(SIGQUIT)");
    }
}

void reset_child_signals(void) {
    struct sigaction sa_dfl;
    memset(&sa_dfl, 0, sizeof(sa_dfl));
    sa_dfl.sa_handler = SIG_DFL;
    sigemptyset(&sa_dfl.sa_mask);
    sigaction(SIGINT, &sa_dfl, NULL);
    sigaction(SIGQUIT, &sa_dfl, NULL);
}

static void render_prompt(const ShellState *state) {
    char cwd[PATH_MAX];
    const char *dir_display = "?";

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        dir_display = cwd;
    }

    const char *home = getenv("HOME");
    char short_cwd[PATH_MAX];
    if (home != NULL && home[0] != '\0') {
        size_t home_len = strlen(home);
        if (strncmp(dir_display, home, home_len) == 0 &&
            (dir_display[home_len] == '\0' || dir_display[home_len] == '/')) {
            snprintf(short_cwd, sizeof(short_cwd), "~%s", dir_display + home_len);
            dir_display = short_cwd;
        }
    }

    bool use_color = isatty(STDOUT_FILENO) != 0;
    if (state->trace_syscalls) {
        if (use_color) {
            printf("\033[0;36m[trace]\033[0m ");
        } else {
            printf("[trace] ");
        }
    }
    if (state->last_status != 0) {
        if (use_color) {
            printf("\033[1;31m[%d]\033[0m ", state->last_status);
        } else {
            printf("[%d] ", state->last_status);
        }
    }

    if (use_color) {
        printf("\033[1;32mjsh\033[0m:\033[1;34m%s\033[0m$ ", dir_display);
    } else {
        printf("jsh:%s$ ", dir_display);
    }
    fflush(stdout);
}

static bool is_blank_line(const char *line) {
    for (size_t i = 0; line[i] != '\0'; i++) {
        if (!isspace((unsigned char)line[i])) {
            return false;
        }
    }
    return true;
}

InputResult read_command_line(char **line_out, size_t *cap_out, const ShellState *state) {
    bool interactive = isatty(STDIN_FILENO) != 0;
    g_sigint_received = 0;

    if (interactive) {
        render_prompt(state);
    }

    errno = 0;
    ssize_t nread = getline(line_out, cap_out, stdin);
    if (nread == -1) {
        if (errno == EINTR || g_sigint_received) {
            clearerr(stdin);
            if (interactive) {
                putchar('\n');
            }
            return INPUT_INTERRUPTED;
        }
        if (feof(stdin)) {
            if (interactive) {
                putchar('\n');
            }
            return INPUT_EOF;
        }
        jsh_perror("getline");
        return INPUT_EOF;
    }

    if (is_blank_line(*line_out)) {
        return INPUT_EMPTY;
    }

    return INPUT_OK;
}
