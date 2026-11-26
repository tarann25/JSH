#include "error.h"
#include "executor.h"
#include "input.h"
#include "parser.h"
#include "shell.h"

#include <stdlib.h>

int main(void) {
    ShellState state = {
        .last_status = JSH_SUCCESS,
        .should_exit = false,
        .exit_code = JSH_SUCCESS,
        .prev_dir = NULL,
        .trace_syscalls = false
    };

    install_interactive_signals();

    char *line = NULL;
    size_t line_cap = 0;

    while (!state.should_exit) {
        InputResult in_res = read_command_line(&line, &line_cap, &state);
        if (in_res == INPUT_EOF) {
            state.exit_code = state.last_status;
            break;
        }
        if (in_res == INPUT_EMPTY || in_res == INPUT_INTERRUPTED) {
            continue;
        }

        Pipeline pipeline = {NULL, 0};
        ParseResult parse_res = parse_line(line, &pipeline);
        if (parse_res == PARSE_EMPTY) {
            continue;
        }
        if (parse_res == PARSE_SYNTAX_ERROR) {
            state.last_status = 2;
            continue;
        }

        jsh_trace(state.trace_syscalls, "parse: %zu command stage(s)", pipeline.count);
        execute_pipeline_or_command(&pipeline, &state);
        free_pipeline(&pipeline);
    }

    free(line);
    free(state.prev_dir);
    return state.exit_code;
}
