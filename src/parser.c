#include "parser.h"
#include "error.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOK_WORD = 0,
    TOK_PIPE,
    TOK_REDIR_IN,
    TOK_REDIR_OUT,
    TOK_REDIR_APPEND
} TokenType;

typedef struct {
    TokenType type;
    char *text;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenList;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StrBuf;

static void strbuf_append(StrBuf *buf, char ch) {
    if (buf->len + 1 >= buf->cap) {
        buf->cap = buf->cap == 0 ? 32 : buf->cap * 2;
        buf->data = jsh_realloc(buf->data, buf->cap);
    }
    buf->data[buf->len++] = ch;
    buf->data[buf->len] = '\0';
}

static void token_list_push(TokenList *list, TokenType type, char *text) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        list->items = jsh_realloc(list->items, list->capacity * sizeof(Token));
    }
    list->items[list->count].type = type;
    list->items[list->count].text = text;
    list->count++;
}

static void token_list_free(TokenList *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i].text);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static const char *token_label(TokenType type) {
    switch (type) {
    case TOK_PIPE:
        return "|";
    case TOK_REDIR_IN:
        return "<";
    case TOK_REDIR_OUT:
        return ">";
    case TOK_REDIR_APPEND:
        return ">>";
    case TOK_WORD:
    default:
        return "word";
    }
}

static bool tokenize(const char *line, TokenList *tokens) {
    size_t i = 0;
    while (line[i] != '\0') {
        while (line[i] != '\0' && isspace((unsigned char)line[i])) {
            i++;
        }
        if (line[i] == '\0') {
            break;
        }

        if (line[i] == '|') {
            token_list_push(tokens, TOK_PIPE, NULL);
            i++;
            continue;
        }
        if (line[i] == '<') {
            token_list_push(tokens, TOK_REDIR_IN, NULL);
            i++;
            continue;
        }
        if (line[i] == '>') {
            if (line[i + 1] == '>') {
                token_list_push(tokens, TOK_REDIR_APPEND, NULL);
                i += 2;
            } else {
                token_list_push(tokens, TOK_REDIR_OUT, NULL);
                i++;
            }
            continue;
        }

        StrBuf buf = {NULL, 0, 0};
        /* Track whether quotes appeared so "" produces an empty argument */
        bool has_word = false;
        char quote = '\0';

        while (line[i] != '\0') {
            char ch = line[i];
            if (quote != '\0') {
                if (ch == quote) {
                    quote = '\0';
                    i++;
                } else if (quote == '"' && ch == '\\' &&
                           (line[i + 1] == '"' || line[i + 1] == '\\')) {
                    strbuf_append(&buf, line[i + 1]);
                    i += 2;
                } else {
                    strbuf_append(&buf, ch);
                    i++;
                }
            } else {
                if (isspace((unsigned char)ch) || ch == '|' || ch == '<' || ch == '>') {
                    break;
                }
                if (ch == '\'' || ch == '"') {
                    quote = ch;
                    has_word = true;
                    i++;
                } else {
                    strbuf_append(&buf, ch);
                    has_word = true;
                    i++;
                }
            }
        }

        if (quote != '\0') {
            free(buf.data);
            jsh_error("syntax error: unterminated %c quote", quote);
            return false;
        }

        if (has_word) {
            if (buf.data == NULL) {
                buf.data = jsh_strdup("");
            }
            token_list_push(tokens, TOK_WORD, buf.data);
        }
    }

    return true;
}

static void command_add_arg(Command *cmd, size_t *cap, const char *arg) {
    if (cmd->argc + 1 >= *cap) {
        *cap = (*cap == 0) ? 8 : (*cap * 2);
        cmd->argv = jsh_realloc(cmd->argv, *cap * sizeof(char *));
    }
    cmd->argv[cmd->argc++] = jsh_strdup(arg);
    cmd->argv[cmd->argc] = NULL;
}

static void init_command(Command *cmd) {
    cmd->argv = NULL;
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = false;
}

ParseResult parse_line(const char *line, Pipeline *out_pipeline) {
    out_pipeline->commands = NULL;
    out_pipeline->count = 0;

    if (line == NULL) {
        return PARSE_EMPTY;
    }

    TokenList tokens = {NULL, 0, 0};
    if (!tokenize(line, &tokens)) {
        token_list_free(&tokens);
        return PARSE_SYNTAX_ERROR;
    }

    if (tokens.count == 0) {
        token_list_free(&tokens);
        return PARSE_EMPTY;
    }

    size_t cmd_cap = 4;
    out_pipeline->commands = jsh_calloc(cmd_cap, sizeof(Command));
    out_pipeline->count = 1;
    init_command(&out_pipeline->commands[0]);
    size_t argv_cap = 0;

    for (size_t i = 0; i < tokens.count; i++) {
        Token *tok = &tokens.items[i];
        Command *curr = &out_pipeline->commands[out_pipeline->count - 1];

        if (tok->type == TOK_WORD) {
            command_add_arg(curr, &argv_cap, tok->text);
            continue;
        }

        if (tok->type == TOK_PIPE) {
            if (curr->argc == 0) {
                jsh_error("syntax error near unexpected token '|'");
                goto syntax_err;
            }
            if (i + 1 >= tokens.count || tokens.items[i + 1].type == TOK_PIPE) {
                jsh_error("syntax error near unexpected token '|'");
                goto syntax_err;
            }
            if (out_pipeline->count >= cmd_cap) {
                cmd_cap *= 2;
                out_pipeline->commands =
                    jsh_realloc(out_pipeline->commands, cmd_cap * sizeof(Command));
            }
            Command *next = &out_pipeline->commands[out_pipeline->count++];
            init_command(next);
            argv_cap = 0;
            continue;
        }

        /* Redirection operator: must be followed by a filename word token */
        if (i + 1 >= tokens.count) {
            jsh_error("syntax error: missing filename after '%s'", token_label(tok->type));
            goto syntax_err;
        }
        Token *target = &tokens.items[++i];
        if (target->type != TOK_WORD) {
            jsh_error("syntax error near unexpected token '%s'", token_label(target->type));
            goto syntax_err;
        }

        if (tok->type == TOK_REDIR_IN) {
            free(curr->input_file);
            curr->input_file = jsh_strdup(target->text);
        } else {
            free(curr->output_file);
            curr->output_file = jsh_strdup(target->text);
            curr->append_output = (tok->type == TOK_REDIR_APPEND);
        }
    }

    if (out_pipeline->commands[out_pipeline->count - 1].argc == 0) {
        jsh_error("syntax error: missing command");
        goto syntax_err;
    }

    token_list_free(&tokens);
    return PARSE_OK;

syntax_err:
    token_list_free(&tokens);
    free_pipeline(out_pipeline);
    return PARSE_SYNTAX_ERROR;
}

void free_pipeline(Pipeline *pipeline) {
    if (pipeline == NULL || pipeline->commands == NULL) {
        return;
    }
    for (size_t i = 0; i < pipeline->count; i++) {
        Command *cmd = &pipeline->commands[i];
        if (cmd->argv != NULL) {
            for (size_t j = 0; j < cmd->argc; j++) {
                free(cmd->argv[j]);
            }
            free(cmd->argv);
        }
        free(cmd->input_file);
        free(cmd->output_file);
    }
    free(pipeline->commands);
    pipeline->commands = NULL;
    pipeline->count = 0;
}
