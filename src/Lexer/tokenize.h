// == CHANGES : ==
// - TokenType enum: added TOKEN_CHANGE and TOKEN_NAME (right after
//   TOKEN_CLEAR) for the new "change name" command keywords.
#ifndef SRC_TOKENIZE_H_
#define SRC_TOKENIZE_H_

#define MAX_TOKENS_NUM_PER_COMMAND 64

typedef enum {
    TOKEN_EOF, // defaults to 0 (good fo init)
    TOKEN_CREATE,
    TOKEN_RELATIONSHIP,
    TOKEN_ENTITY,
    TOKEN_ADD,
    TOKEN_PROPERTY,
    TOKEN_CARDINALITY,
    TOKEN_KEY,
    TOKEN_FK, // Foreign key
    TOKEN_PK, // Primary  key
    TOKEN_CONVERT,
    TOKEN_MLD,
    TOKEN_SQL,
    TOKEN_DELETE,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_HELP,
    TOKEN_CLEAR,
    TOKEN_CHANGE,
    TOKEN_NAME,
    TOKEN_INT_TYPE,
    TOKEN_STRING_TYPE,
    TOKEN_DOUBLE_TYPE,
    TOKEN_DATE_TYPE,
    TOKEN_MONEY_TYPE,
    TOKEN_UNKNOWN // for weird unicode chars like ~ or @
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
void advance_token(int *current);
Token peek_token(Token *tokens, int current);
#endif //
