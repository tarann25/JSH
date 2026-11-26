CC      := gcc
CFLAGS  := -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -pedantic -Iinclude
DBGFLAGS:= -g -O1 -fno-omit-frame-pointer

SRCS := src/main.c \
        src/input.c \
        src/parser.c \
        src/builtins.c \
        src/redirect.c \
        src/executor.c \
        src/pipeline.c \
        src/error.c

OBJS := $(SRCS:src/%.c=build/%.o)

TARGET      := jsh
TEST_PARSER := build/test_parser

.PHONY: all debug test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_PARSER): tests/test_parser.c src/parser.c src/error.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^

debug:
	@mkdir -p build
	$(CC) $(CFLAGS) $(DBGFLAGS) -o jsh_debug $(SRCS)
	$(CC) $(CFLAGS) $(DBGFLAGS) -o build/test_parser_debug tests/test_parser.c src/parser.c src/error.c

test: $(TARGET) $(TEST_PARSER)
	./$(TEST_PARSER)
	bash tests/run_tests.sh ./$(TARGET)

clean:
	rm -rf build $(TARGET) jsh_debug
