# jsh — JIIT Command-Line Shell

A modular Unix/Linux command-line shell implemented from scratch in **C11** using **POSIX system calls**, built for the Operating Systems Lab project (*Shell Building from Scratch*).

## Features

- **Interactive Prompt & Input Reader**: Displays `jsh:<cwd>$ ` (with last non-zero exit status indicator), safely reads arbitrary-length lines with `getline()`, handles blank lines, `Ctrl+D` (`EOF`), and `Ctrl+C` (`SIGINT`).
- **Command & Quote Parser**: Splits whitespace-separated tokens, supports glued operators (`ls|wc -l`, `echo hi>out.txt`), single/double quote grouping (`'...'`, `"..."`), and reports syntax errors cleanly.
- **Built-in Commands**:
  - `cd [dir | - | ~]` — changes working directory inside the parent shell process via `chdir()`.
  - `pwd` — prints current working directory via `getcwd()`.
  - `trace [on | off]` — toggles live POSIX syscall, PID, and file-descriptor trace output for OS lab demonstrations.
  - `exit [code]` — exits cleanly with optional numeric status.
  - `help` — prints a concise command & syntax reference.
- **External Program Execution**: Creates child processes with `fork()`, executes commands via `execvp()`, and waits synchronously with `waitpid()`.
- **I/O Redirection**: Supports `<` (input), `>` (truncate output), and `>>` (append output) via `open()`, `dup2()`, and `close()` for both external commands and built-ins.
- **Multi-Stage Pipelines**: Connects arbitrary chains of commands (`cmd1 | cmd2 | cmd3`) using `pipe()`, `dup2()`, strict unused descriptor cleanup, and full child reaping.

## Project Structure (Aligned with Synopsis Sections 5 & 6)

| Module | Synopsis Section | Source Files |
|---|---|---|
| Input Reader | 5.1 | `include/input.h`, `src/input.c` |
| Command Parser | 5.2 | `include/parser.h`, `src/parser.c` |
| Built-in Handler | 5.3 | `include/builtins.h`, `src/builtins.c` |
| Process Execution | 5.4 | `include/executor.h`, `src/executor.c` |
| Redirection Handler | 5.5 | `include/redirect.h`, `src/redirect.c` |
| Pipeline Handler | 5.6 | `include/pipeline.h`, `src/pipeline.c` |
| Error Handling | 5.7 | `include/error.h`, `src/error.c` |
| Main Workflow Loop | 6 | `include/shell.h`, `src/main.c` |

## Build, Run & Test

```bash
make            # Build ./jsh
./jsh           # Start interactive shell
make test       # Run parser unit tests and T1-T9 integration tests
make debug      # Build ASan/UBSan instrumented binaries
make clean      # Remove build outputs
```
