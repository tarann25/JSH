#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_simple_command(void) {
    Pipeline p = {NULL, 0};
    assert(parse_line("  ls   -la   /tmp \n", &p) == PARSE_OK);
    assert(p.count == 1);
    assert(p.commands[0].argc == 3);
    assert(strcmp(p.commands[0].argv[0], "ls") == 0);
    assert(strcmp(p.commands[0].argv[1], "-la") == 0);
    assert(strcmp(p.commands[0].argv[2], "/tmp") == 0);
    assert(p.commands[0].argv[3] == NULL);
    assert(p.commands[0].input_file == NULL);
    assert(p.commands[0].output_file == NULL);
    free_pipeline(&p);
}

static void test_empty_lines(void) {
    Pipeline p = {NULL, 0};
    assert(parse_line("", &p) == PARSE_EMPTY);
    assert(parse_line("   \t\n", &p) == PARSE_EMPTY);
    assert(parse_line(NULL, &p) == PARSE_EMPTY);
}

static void test_redirections_and_glued_ops(void) {
    Pipeline p = {NULL, 0};
    assert(parse_line("cat<in.txt>>out.txt", &p) == PARSE_OK);
    assert(p.count == 1);
    assert(p.commands[0].argc == 1);
    assert(strcmp(p.commands[0].argv[0], "cat") == 0);
    assert(strcmp(p.commands[0].input_file, "in.txt") == 0);
    assert(strcmp(p.commands[0].output_file, "out.txt") == 0);
    assert(p.commands[0].append_output == true);
    free_pipeline(&p);

    assert(parse_line("echo hello > output.txt", &p) == PARSE_OK);
    assert(p.count == 1);
    assert(p.commands[0].argc == 2);
    assert(strcmp(p.commands[0].argv[0], "echo") == 0);
    assert(strcmp(p.commands[0].argv[1], "hello") == 0);
    assert(strcmp(p.commands[0].output_file, "output.txt") == 0);
    assert(p.commands[0].append_output == false);
    free_pipeline(&p);
}

static void test_multi_pipeline(void) {
    Pipeline p = {NULL, 0};
    assert(parse_line("ls -la|grep src | wc -l", &p) == PARSE_OK);
    assert(p.count == 3);
    assert(p.commands[0].argc == 2);
    assert(strcmp(p.commands[0].argv[0], "ls") == 0);
    assert(strcmp(p.commands[1].argv[0], "grep") == 0);
    assert(strcmp(p.commands[2].argv[0], "wc") == 0);
    assert(strcmp(p.commands[2].argv[1], "-l") == 0);
    free_pipeline(&p);
}

static void test_quotes(void) {
    Pipeline p = {NULL, 0};
    assert(parse_line("echo \"hello world\" 'a|b>c' \"\"", &p) == PARSE_OK);
    assert(p.count == 1);
    assert(p.commands[0].argc == 4);
    assert(strcmp(p.commands[0].argv[0], "echo") == 0);
    assert(strcmp(p.commands[0].argv[1], "hello world") == 0);
    assert(strcmp(p.commands[0].argv[2], "a|b>c") == 0);
    assert(strcmp(p.commands[0].argv[3], "") == 0);
    free_pipeline(&p);
}

static void test_syntax_errors(void) {
    const char *bad_inputs[] = {
        "| ls",
        "ls |",
        "ls || wc",
        "echo >",
        "<",
        "> out.txt",
        "echo > | cat",
        "echo \"unterminated",
        "echo 'unterminated"
    };

    for (size_t i = 0; i < sizeof(bad_inputs) / sizeof(bad_inputs[0]); i++) {
        Pipeline p = {NULL, 0};
        assert(parse_line(bad_inputs[i], &p) == PARSE_SYNTAX_ERROR);
        assert(p.commands == NULL);
        assert(p.count == 0);
    }
}

int main(void) {
    test_simple_command();
    test_empty_lines();
    test_redirections_and_glued_ops();
    test_multi_pipeline();
    test_quotes();
    test_syntax_errors();
    puts("test_parser: all unit tests passed");
    return 0;
}
