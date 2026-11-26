#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

typedef enum {
    PARSE_OK = 0,
    PARSE_EMPTY,
    PARSE_SYNTAX_ERROR
} ParseResult;

/**
 * Parse a raw command line into a Pipeline of Commands.
 * On PARSE_OK, caller must free the result with free_pipeline().
 */
ParseResult parse_line(const char *line, Pipeline *out_pipeline);

/** Free all heap memory associated with a parsed Pipeline. */
void free_pipeline(Pipeline *pipeline);

#endif /* PARSER_H */
