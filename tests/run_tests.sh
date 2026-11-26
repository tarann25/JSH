#!/usr/bin/env bash
set -euo pipefail

SHELL_BIN="${1:-./jsh}"
TMPDIR_TEST="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_TEST"' EXIT

pass_count=0
fail_count=0

pass() {
    echo "PASS: $1"
    pass_count=$((pass_count + 1))
}

fail() {
    echo "FAIL: $1" >&2
    fail_count=$((fail_count + 1))
}

# T1 - Built-in pwd
out="$(printf 'pwd\nexit\n' | "$SHELL_BIN")"
expected="$(pwd)"
if [[ "$out" == "$expected" ]]; then
    pass "T1 - Built-in pwd"
else
    fail "T1 - Built-in pwd (got '$out', expected '$expected')"
fi

# T2 - Change directory (cd, pwd, cd -)
out="$(printf 'cd %s\npwd\ncd -\npwd\nexit\n' "$TMPDIR_TEST" | "$SHELL_BIN")"
expected_t2="$(printf '%s\n%s\n%s' "$TMPDIR_TEST" "$expected" "$expected")"
if [[ "$out" == "$expected_t2" ]]; then
    pass "T2 - Built-in cd and cd -"
else
    fail "T2 - Built-in cd and cd - (got '$out')"
fi

# T3 - Exit (clean termination & custom exit status)
set +e
printf 'exit 42\n' | "$SHELL_BIN"
rc=$?
set -e
if [[ "$rc" -eq 42 ]]; then
    pass "T3 - Clean exit with status"
else
    fail "T3 - Clean exit with status (got $rc, expected 42)"
fi

# T4 - External command + quoted args + blank lines
out="$(printf '\n   \necho hello "world 123"\nexit\n' | "$SHELL_BIN")"
if [[ "$out" == "hello world 123" ]]; then
    pass "T4 - External command & blank lines"
else
    fail "T4 - External command & blank lines (got '$out')"
fi

# T5 - Unknown command continues shell
err_file="$TMPDIR_TEST/t5.err"
out="$(printf 'nonexistent_cmd_xyz_123\necho still_alive\nexit\n' | "$SHELL_BIN" 2>"$err_file")"
if [[ "$out" == "still_alive" ]] && grep -q "command not found" "$err_file"; then
    pass "T5 - Unknown command error handling"
else
    fail "T5 - Unknown command error handling (out='$out', err='$(cat "$err_file")')"
fi

# T6 - Output redirection (> and >>)
out_file="$TMPDIR_TEST/output.txt"
printf 'echo hello > %s\necho world >> %s\nexit\n' "$out_file" "$out_file" | "$SHELL_BIN"
if [[ "$(cat "$out_file")" == $'hello\nworld' ]]; then
    pass "T6 - Output redirection (> and >>)"
else
    fail "T6 - Output redirection (got '$(cat "$out_file")')"
fi

# T7 - Input redirection (<) and missing file error
in_file="$TMPDIR_TEST/input.txt"
printf 'alpha\nbeta\ngamma\n' > "$in_file"
out="$(printf 'wc -l < %s\nexit\n' "$in_file" | "$SHELL_BIN" | tr -d ' ')"
if [[ "$out" == "3" ]]; then
    pass "T7 - Input redirection (<)"
else
    fail "T7 - Input redirection (got '$out')"
fi

# T8 - Two-command and multi-stage pipelines + pipeline redirection
pipe_out="$TMPDIR_TEST/pipe_out.txt"
out="$(printf 'printf "b\\na\\nc\\n" | sort | head -n 2\nexit\n' | "$SHELL_BIN")"
printf 'ls src | grep main > %s\nexit\n' "$pipe_out" | "$SHELL_BIN"
if [[ "$out" == $'a\nb' ]] && [[ "$(cat "$pipe_out")" == "main.c" ]]; then
    pass "T8 - Two-stage & multi-stage pipelines"
else
    fail "T8 - Pipelines (out='$out', pipe_out='$(cat "$pipe_out")')"
fi

# T9 - Resource cleanup (no FD leaks across repeated redirections & pipelines)
fd_before_file="$TMPDIR_TEST/fd_before.txt"
fd_after_file="$TMPDIR_TEST/fd_after.txt"
{
    printf 'ls /proc/self/fd | wc -l > %s\n' "$fd_before_file"
    for i in $(seq 1 20); do
        printf 'echo line_%d > %s/tmp_%d.txt\n' "$i" "$TMPDIR_TEST" "$i"
        printf 'cat < %s/tmp_%d.txt | wc -c > /dev/null\n' "$TMPDIR_TEST" "$i"
    done
    printf 'ls /proc/self/fd | wc -l > %s\n' "$fd_after_file"
    printf 'exit\n'
} | "$SHELL_BIN"

fd_before="$(tr -d ' ' < "$fd_before_file")"
fd_after="$(tr -d ' ' < "$fd_after_file")"
if [[ "$fd_before" == "$fd_after" ]]; then
    pass "T9 - Resource cleanup (open FDs unchanged: $fd_before -> $fd_after)"
else
    fail "T9 - Resource cleanup (FD count changed: $fd_before -> $fd_after)"
fi

# Extra: Built-in with redirection & syntax error recovery
builtin_out="$TMPDIR_TEST/pwd.txt"
err_file="$TMPDIR_TEST/syntax.err"
out="$(printf 'pwd > %s\n| invalid\necho recovered\nexit\n' "$builtin_out" | "$SHELL_BIN" 2>"$err_file")"
if [[ "$(cat "$builtin_out")" == "$expected" ]] && [[ "$out" == "recovered" ]] && grep -q "syntax error" "$err_file"; then
    pass "Extra - Built-in redirection and syntax error recovery"
else
    fail "Extra - Built-in redirection/syntax recovery"
fi

# Extra: Live syscall tracing (trace on / trace off)
trace_err="$TMPDIR_TEST/trace.err"
trace_file="$TMPDIR_TEST/traced.txt"
out="$(printf 'trace on\necho hi > %s\nls src | wc -l\ntrace off\necho quiet\nexit\n' "$trace_file" | "$SHELL_BIN" 2>"$trace_err")"
if grep -q "\[trace\] pipe()" "$trace_err" && \
   grep -q "\[trace\] fork()" "$trace_err" && \
   grep -q "open(" "$trace_err" && \
   grep -q "dup2(" "$trace_err" && \
   grep -q "execvp(" "$trace_err" && \
   grep -q "waitpid(" "$trace_err" && \
   echo "$out" | grep -q "trace disabled"; then
    pass "Extra - Live syscall/FD visualizer (trace on / trace off)"
else
    fail "Extra - Live syscall/FD visualizer (err='$(cat "$trace_err")')"
fi

echo "Summary: $pass_count passed, $fail_count failed"
[[ "$fail_count" -eq 0 ]]
