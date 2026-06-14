#include "tokenize.h"
#include <ctype.h>
#include <string.h>

const char *skip_whitespace(const char *p, const char *content) {
    while (isspace(*p)) {
        p++;
    }
    return p;
}
void tokenize_content(const char *content, Token tokens[], int *count) {
    if (!content) {
        // No content
        *count = 0;
        return;
    }

    // count is init by parser_init() to 0
    const char *p = content;
    while (*p != '\0' && *count < 63) {
        p = skip_whitespace(p, content);
        if (*p == '\0')
            break;

        // handle string token

        if (*p == '"' || *p == '\'') {
            // skip the quotations
            p++;
            int len = 0;
            // check entity / relationship domain integrity length in
            // MCDelements.h

            while (*p != '\0' && *p != '"' && *p != '\'' && len < 63) {
                tokens[*count].value[len++] = *p++;
            }
            if (*p == '"' || *p == '\'') {
                p++;
            } else {
                goto EOF;
                // TODO: implement a better fix for missing closing quotations
            }
            tokens[*count].pos = (int)(p - content);
            tokens[*count].length = len;
            tokens[*count].value[len] = '\0';
            tokens[*count].type = TOKEN_STRING;
        } else if (isalnum(*p)) { // keyword or identifier
            int len = 0;
            while (*p != '\0' && isalnum(*p) && len < 63) {
                tokens[*count].value[len++] = *p++;
            }

            // position of  the start of the token for error reporting
            tokens[*count].pos = (int)(p - content);
            tokens[*count].length = len;
            tokens[*count].value[len] = '\0';

            if (strcmp(tokens[*count].value, "create") == 0) {
                tokens[*count].type = TOKEN_CREATE;
            } else if (strcmp(tokens[*count].value, "entity") == 0) {
                tokens[*count].type = TOKEN_ENTITY;
            } else if (strcmp(tokens[*count].value, "relationship") == 0) {
                tokens[*count].type = TOKEN_RELATIONSHIP;
            } else if (strcmp(tokens[*count].value, "add") == 0) {
                tokens[*count].type = TOKEN_ADD;
            } else if (strcmp(tokens[*count].value, "property") == 0) {
                tokens[*count].type = TOKEN_PROPERTY;
            } else if (strcmp(tokens[*count].value, "clear") == 0) {
                tokens[*count].type = TOKEN_CLEAR;
            } else if (strcmp(tokens[*count].value, "money") == 0) {
                tokens[*count].type = TOKEN_MONEY_TYPE;
            } else if (strcmp(tokens[*count].value, "int") == 0) {
                tokens[*count].type = TOKEN_INT_TYPE;
            } else if (strcmp(tokens[*count].value, "str") == 0) {
                tokens[*count].type = TOKEN_STRING_TYPE;
            } else if (strcmp(tokens[*count].value, "double") == 0) {
                tokens[*count].type = TOKEN_DOUBLE_TYPE;
            } else if (strcmp(tokens[*count].value, "date") == 0) {
                tokens[*count].type = TOKEN_DATE_TYPE;
            } else {
                // testing
                tokens[*count].type = TOKEN_IDENTIFIER;
            }
        } else { // Other Symbols like tilde or @
            tokens[*count].pos = (int)(p - content);
            tokens[*count].length = 1;
            tokens[*count].value[1] = '\0';
            tokens[*count].type = TOKEN_UNKNOWN;
            p++;
        }
        (*count)++;
    }

    // Set the position !!
EOF:
    tokens[*count].pos = (int)(p - content);
    tokens[*count].length = 1;
    tokens[*count].type = TOKEN_EOF;
    tokens[*count].value[0] = '\0';
}

Token get_next_token(Token *tokens, int *current, int count) {
    if (count == 0) {
        // potential error in tokenization
        return (Token){.type = TOKEN_EOF, .value[0] = '\0', .length = 1};
    }

    if (*current != count - 1) {
        return tokens[(*current)++];
    }
    return (Token){.type = TOKEN_EOF, .value[0] = '\0', .length = 1};
}

void advance_token(int *current) { *current += 1; }
Token peek_token(Token *tokens, int current) { return tokens[current]; }
