#ifndef ANALYZER_H
#define ANALYZER_H

#include <stddef.h>

// Token types for lexical analysis
typedef enum {
    TOKEN_CODE,              // Regular code (identifiers, literals, other operators)
    TOKEN_STRING,            // String literal
    TOKEN_BLOCK_COMMENT,     // /* */ comment
    TOKEN_LINE_COMMENT,      // // comment
    TOKEN_INTERFACE,         // interface keyword
    TOKEN_TYPE,              // type keyword
    TOKEN_IMPLEMENTS,        // implements keyword
    TOKEN_AS,                // as keyword
    TOKEN_PRIVATE,           // private keyword
    TOKEN_COLON,             // : type annotation
    TOKEN_OPTIONAL,          // ?: optional parameter
    TOKEN_LT,                // < (less than or generic start)
    TOKEN_GT,                // > (greater than or generic end)
    TOKEN_EQ,                // = (assignment or type alias)
    TOKEN_EOF                // End of file
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    const char *start;       // Pointer to start in source
    size_t length;           // Length of token
    int line;                // Line number
} Token;

// AST Node list (array of tokens)
typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} AST;

// Lexer: Tokenize source code into AST
AST* lex(const char *source, size_t size);

// Parser: Process AST and strip types
char* parse(const AST *ast, const char *source);

// Free AST memory
void ast_free(AST *ast);

// High-level API: Strip TypeScript/Flow type annotations from JavaScript code
// The caller is responsible for freeing the returned string
char* strip_types(const char *source, size_t size);

#endif // ANALYZER_H
