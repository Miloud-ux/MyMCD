#ifndef SRC_TOKENIZE_H_
#define SRC_TOKENIZE_H_

typedef enum {
    TOKEN_EOF, // defaults to 0 (good fo init)
    TOKEN_CREATE,
    TOKEN_RELATIONSHIP,
    TOKEN_ENTITY,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_HELP,
    TOKEN_CLEAR,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
        char value[64];
        int pos;
        int length;
        TokenType type;
} Token;

void tokenize_content(const char *content, Token tokens[], int *count);
const char *skip_whitespace(const char *p, const char *content);
Token get_next_token(Token *tokens, int *current, int count);

#endif //
