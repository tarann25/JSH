#include "builtins.h"
#include "error.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int builtin_cd(const Command *cmd, ShellState *state) {
    if (cmd->argc > 2) {
        jsh_error("cd: too many arguments");
        return JSH_ERROR;
    }

    const char *target = NULL;
    bool print_new_dir = false;

    if (cmd->argc == 1 || strcmp(cmd->argv[1], "~") == 0) {
        target = getenv("HOME");
        if (target == NULL) {
            jsh_error("cd: HOME not set");
            return JSH_ERROR;
        }
    } else if (strcmp(cmd->argv[1], "-") == 0) {
        if (state->prev_dir == NULL) {
            jsh_error("cd: OLDPWD not set");
            return JSH_ERROR;
        }
        target = state->prev_dir;
        print_new_dir = true;
    } else {
        target = cmd->argv[1];
    }

    char old_cwd[PATH_MAX];
    bool have_old_cwd = (getcwd(old_cwd, sizeof(old_cwd)) != NULL);

    if (chdir(target) != 0) {
        jsh_perror(target);
        return JSH_ERROR;
    }

    jsh_trace(state->trace_syscalls,
              "builtin cd: chdir(\"%s\") in parent pid=%d",
              target, (int)getpid());

    if (have_old_cwd) {
        free(state->prev_dir);
        state->prev_dir = jsh_strdup(old_cwd);
        setenv("OLDPWD", old_cwd, 1);
    }

    char new_cwd[PATH_MAX];
    if (getcwd(new_cwd, sizeof(new_cwd)) != NULL) {
        setenv("PWD", new_cwd, 1);
        if (print_new_dir) {
            printf("%s\n", new_cwd);
        }
    } else if (print_new_dir) {
        printf("%s\n", target);
    }

    return JSH_SUCCESS;
}

static int builtin_pwd(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        jsh_perror("pwd");
        return JSH_ERROR;
    }
    printf("%s\n", cwd);
    return JSH_SUCCESS;
}

static int builtin_exit(const Command *cmd, ShellState *state) {
    if (cmd->argc > 2) {
        jsh_error("exit: too many arguments");
        return JSH_ERROR;
    }

    if (cmd->argc == 2) {
        char *endptr = NULL;
        errno = 0;
        long code = strtol(cmd->argv[1], &endptr, 10);
        if (errno == ERANGE || endptr == cmd->argv[1] || *endptr != '\0') {
            jsh_error("exit: %s: numeric argument required", cmd->argv[1]);
            state->exit_code = 2;
            state->should_exit = true;
            return 2;
        }
        state->exit_code = (int)(code & 0xFF);
    } else {
        state->exit_code = state->last_status;
    }

    state->should_exit = true;
    return state->exit_code;
}

static int builtin_trace(const Command *cmd, ShellState *state) {
    if (cmd->argc > 2) {
        jsh_error("trace: usage: trace [on|off]");
        return JSH_ERROR;
    }
    if (cmd->argc == 1) {
        printf("trace is currently %s\n", state->trace_syscalls ? "on" : "off");
        return JSH_SUCCESS;
    }
    if (strcmp(cmd->argv[1], "on") == 0) {
        state->trace_syscalls = true;
        jsh_trace(true, "syscall & file-descriptor tracing enabled (shell pid=%d)", (int)getpid());
        return JSH_SUCCESS;
    }
    if (strcmp(cmd->argv[1], "off") == 0) {
        state->trace_syscalls = false;
        puts("trace disabled");
        return JSH_SUCCESS;
    }

    jsh_error("trace: invalid option '%s' (use 'on' or 'off')", cmd->argv[1]);
    return JSH_ERROR;
}

static int builtin_help(void) {
    puts("jsh - JIIT Simple Command-Line Shell");
    puts("Built-in commands:");
    puts("  cd [dir | ~ | -]   Change the shell working directory");
    puts("  pwd                Print the current working directory");
    puts("  trace [on | off]   Toggle live POSIX syscall/FD trace output");
    puts("  help               Display this help summary");
    puts("  exit [code]        Exit the shell cleanly");
    puts("Supported operators:");
    puts("  cmd < file         Redirect standard input from file");
    puts("  cmd > file         Redirect standard output to file (overwrite)");
    puts("  cmd >> file        Redirect standard output to file (append)");
    puts("  c1 | c2 [| c3...]  Connect commands via POSIX pipelines");
    puts("  '...' / \"...\"      Group arguments containing whitespace");
    return JSH_SUCCESS;
}

bool is_builtin(const char *cmd_name) {
    if (cmd_name == NULL) {
        return false;
    }
    return strcmp(cmd_name, "cd") == 0 ||
           strcmp(cmd_name, "pwd") == 0 ||
           strcmp(cmd_name, "exit") == 0 ||
           strcmp(cmd_name, "trace") == 0 ||
           strcmp(cmd_name, "help") == 0;
}

int execute_builtin(const Command *cmd, ShellState *state) {
    const char *name = cmd->argv[0];
    int status = JSH_SUCCESS;

    if (strcmp(name, "cd") == 0) {
        status = builtin_cd(cmd, state);
    } else if (strcmp(name, "pwd") == 0) {
        status = builtin_pwd();
    } else if (strcmp(name, "exit") == 0) {
        status = builtin_exit(cmd, state);
    } else if (strcmp(name, "trace") == 0) {
        status = builtin_trace(cmd, state);
    } else if (strcmp(name, "help") == 0) {
        status = builtin_help();
    } else {
        jsh_error("%s: unknown built-in", name);
        status = JSH_ERROR;
    }

    state->last_status = status;
    return status;
}
