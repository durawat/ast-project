#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct {
    char *buffer;
    size_t size;
    size_t capacity;
} StringBuilder;

StringBuilder* sb_create(size_t initial_capacity) {
    StringBuilder *sb = malloc(sizeof(StringBuilder));
    if (!sb) return NULL;

    sb->buffer = malloc(initial_capacity);
    if (!sb->buffer) {
        free(sb);
        return NULL;
    }

    sb->buffer[0] = '\0';
    sb->size = 0;
    sb->capacity = initial_capacity;
    return sb;
}

void sb_append(StringBuilder *sb, const char *str) {
    size_t len = strlen(str);
    
    while (sb->size + len >= sb->capacity) {
        sb->capacity *= 2;
        char *new_buffer = realloc(sb->buffer, sb->capacity);
        if (!new_buffer) return;
        sb->buffer = new_buffer;
    }

    strcpy(sb->buffer + sb->size, str);
    sb->size += len;
}

void sb_append_format(StringBuilder *sb, const char *format, ...) {
    char temp[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    sb_append(sb, temp);
}

char* sb_to_string(StringBuilder *sb) {
    char *result = sb->buffer;
    free(sb);
    return result;
}

// ============================================================================
// LEXER: Tokenize source code into AST
// ============================================================================

static void ast_add_token(AST *ast, TokenType type, const char *start, size_t length, int line) {
    if (ast->count >= ast->capacity) {
        ast->capacity *= 2;
        Token *new_tokens = realloc(ast->tokens, ast->capacity * sizeof(Token));
        if (!new_tokens) return;
        ast->tokens = new_tokens;
    }
    
    ast->tokens[ast->count].type = type;
    ast->tokens[ast->count].start = start;
    ast->tokens[ast->count].length = length;
    ast->tokens[ast->count].line = line;
    ast->count++;
}

AST* lex(const char *source, size_t size) {
    if (!source || size == 0) {
        return NULL;
    }
    
    AST *ast = malloc(sizeof(AST));
    if (!ast) return NULL;
    
    ast->capacity = 256;
    ast->count = 0;
    ast->tokens = malloc(ast->capacity * sizeof(Token));
    if (!ast->tokens) {
        free(ast);
        return NULL;
    }
    
    const char *ptr = source;
    const char *end = source + size;
    int line = 1;
    
    typedef enum {
        STATE_CODE,
        STATE_STRING,
        STATE_BLOCK_COMMENT,
        STATE_LINE_COMMENT
    } LexerState;
    
    LexerState state = STATE_CODE;
    char string_delimiter = 0;
    const char *token_start = ptr;
    
    while (ptr < end) {
        char current = *ptr;
        char next = (ptr + 1 < end) ? *(ptr + 1) : '\0';
        
        switch (state) {
            case STATE_CODE:
                // String literals
                if ((current == '"' || current == '\'' || current == '`') && 
                    (ptr == source || *(ptr - 1) != '\\')) {
                    state = STATE_STRING;
                    string_delimiter = current;
                    token_start = ptr;
                    ptr++;
                    continue;
                }
                
                // Block comments
                if (current == '/' && next == '*') {
                    state = STATE_BLOCK_COMMENT;
                    token_start = ptr;
                    ptr += 2;
                    continue;
                }
                
                // Line comments
                if (current == '/' && next == '/') {
                    state = STATE_LINE_COMMENT;
                    token_start = ptr;
                    ptr += 2;
                    continue;
                }
                
                // Keywords
                if (strncmp(ptr, "interface", 9) == 0 && (ptr + 9 >= end || (!isalnum(*(ptr + 9)) && *(ptr + 9) != '_'))) {
                    ast_add_token(ast, TOKEN_INTERFACE, ptr, 9, line);
                    ptr += 9;
                    continue;
                }
                
                if (strncmp(ptr, "type ", 5) == 0) {
                    ast_add_token(ast, TOKEN_TYPE, ptr, 4, line);
                    ptr += 4;
                    continue;
                }
                
                if (strncmp(ptr, "implements ", 11) == 0) {
                    ast_add_token(ast, TOKEN_IMPLEMENTS, ptr, 10, line);
                    ptr += 10;
                    continue;
                }
                
                if (ptr + 3 < end && ptr > source && *(ptr - 1) == ' ' && 
                    strncmp(ptr, "as ", 3) == 0) {
                    ast_add_token(ast, TOKEN_AS, ptr, 2, line);
                    ptr += 2;
                    continue;
                }
                
                if (strncmp(ptr, "private", 7) == 0 && (ptr + 7 >= end || (!isalnum(*(ptr + 7)) && *(ptr + 7) != '_'))) {
                    ast_add_token(ast, TOKEN_PRIVATE, ptr, 7, line);
                    ptr += 7;
                    continue;
                }
                
                // Patterns
                if (current == '?' && next == ':') {
                    ast_add_token(ast, TOKEN_OPTIONAL, ptr, 2, line);
                    ptr += 2;
                    continue;
                }
                
                if (current == ':') {
                    // Check if it's a type annotation (after identifier/paren/bracket)
                    const char *check = ptr - 1;
                    while (check >= source && isspace(*check)) check--;
                    if (check >= source && (isalnum(*check) || *check == '_' || *check == ')' || *check == ']')) {
                        ast_add_token(ast, TOKEN_COLON, ptr, 1, line);
                        ptr++;
                        continue;
                    }
                }
                
                // Emit specific tokens for operators the parser needs to track
                if (current == '<') {
                    ast_add_token(ast, TOKEN_LT, ptr, 1, line);
                    ptr++;
                    continue;
                }
                
                if (current == '>') {
                    ast_add_token(ast, TOKEN_GT, ptr, 1, line);
                    ptr++;
                    continue;
                }
                
                if (current == '=') {
                    ast_add_token(ast, TOKEN_EQ, ptr, 1, line);
                    ptr++;
                    continue;
                }
                
                // Regular code (identifiers, other operators, literals, etc.)
                token_start = ptr;
                ast_add_token(ast, TOKEN_CODE, token_start, 1, line);
                ptr++;
                break;
                
            case STATE_STRING:
                if (current == string_delimiter && (ptr == source || *(ptr - 1) != '\\')) {
                    ptr++;
                    ast_add_token(ast, TOKEN_STRING, token_start, ptr - token_start, line);
                    state = STATE_CODE;
                } else {
                    ptr++;
                }
                break;
                
            case STATE_BLOCK_COMMENT:
                if (current == '*' && next == '/') {
                    ptr += 2;
                    ast_add_token(ast, TOKEN_BLOCK_COMMENT, token_start, ptr - token_start, line);
                    state = STATE_CODE;
                } else {
                    if (current == '\n') line++;
                    ptr++;
                }
                break;
                
            case STATE_LINE_COMMENT:
                if (current == '\n') {
                    ast_add_token(ast, TOKEN_LINE_COMMENT, token_start, ptr - token_start, line);
                    line++;
                    ptr++;
                    state = STATE_CODE;
                } else {
                    ptr++;
                }
                break;
        }
        
        if (current == '\n' && state == STATE_CODE) {
            line++;
        }
    }
    
    ast_add_token(ast, TOKEN_EOF, ptr, 0, line);
    return ast;
}

// ============================================================================
// PARSER: Process AST and strip types
// ============================================================================

char* parse(const AST *ast, const char *source) {
    if (!ast || !source) {
        return NULL;
    }
    
    StringBuilder *output = sb_create(4096);
    if (!output) {
        return NULL;
    }
    
    for (size_t i = 0; i < ast->count; i++) {
        Token token = ast->tokens[i];
        
        switch (token.type) {
            case TOKEN_STRING:
            case TOKEN_BLOCK_COMMENT:
            case TOKEN_LINE_COMMENT:
            case TOKEN_CODE:
            case TOKEN_GT:
            case TOKEN_EQ:
                // Preserve these tokens (output as-is)
                for (size_t j = 0; j < token.length; j++) {
                    sb_append_format(output, "%c", token.start[j]);
                }
                break;
                
            case TOKEN_LT:
                // Check if this is a generic or comparison operator
                // Look back to see if preceded by identifier/close paren
                {
                    int looks_like_generic = 0;
                    
                    // Look back for identifier (skip whitespace/newlines)
                    if (i > 0) {
                        size_t prev = i - 1;
                        while (prev > 0 && ast->tokens[prev].type == TOKEN_CODE) {
                            char c = *ast->tokens[prev].start;
                            if (!isspace(c) && c != '\n') {
                                // Found non-whitespace - check if identifier-like
                                if (isalnum(c) || c == '_' || c == ')') {
                                    looks_like_generic = 1;
                                }
                                break;
                            }
                            prev--;
                        }
                    }
                    
                    if (looks_like_generic) {
                        // Try to find matching > and see what follows
                        int depth = 1;
                        size_t lookahead = i + 1;
                        int found_match = 0;
                        
                        while (lookahead < ast->count && depth > 0) {
                            if (ast->tokens[lookahead].type == TOKEN_LT) depth++;
                            else if (ast->tokens[lookahead].type == TOKEN_GT) {
                                depth--;
                                if (depth == 0) {
                                    found_match = 1;
                                    break;
                                }
                            }
                            lookahead++;
                        }
                        
                        // Check what follows the closing >
                        if (found_match) {
                            lookahead++; // Move past >
                            // Skip whitespace
                            while (lookahead < ast->count && ast->tokens[lookahead].type == TOKEN_CODE) {
                                char c = *ast->tokens[lookahead].start;
                                if (!isspace(c) && c != '\n') break;
                                lookahead++;
                            }
                            
                            // Generic if followed by (, {, =, or =>
                            if (lookahead < ast->count) {
                                Token follow = ast->tokens[lookahead];
                                if (follow.type == TOKEN_CODE) {
                                    char c = *follow.start;
                                    if (c == '(' || c == '{') {
                                        looks_like_generic = 1;
                                    } else {
                                        looks_like_generic = 0; // Probably comparison
                                    }
                                } else if (follow.type == TOKEN_EQ) {
                                    // Could be => arrow function
                                    if (lookahead + 1 < ast->count && ast->tokens[lookahead + 1].type == TOKEN_GT) {
                                        looks_like_generic = 1;
                                    } else {
                                        looks_like_generic = 1; // Type alias: type Foo<T> =
                                    }
                                }
                            }
                        } else {
                            looks_like_generic = 0; // No matching >, must be comparison
                        }
                    }
                    
                    if (looks_like_generic) {
                        // Skip the generic: < ... >
                        int depth = 1;
                        i++; // Skip the <
                        while (i < ast->count && depth > 0) {
                            if (ast->tokens[i].type == TOKEN_LT) depth++;
                            else if (ast->tokens[i].type == TOKEN_GT) {
                                depth--;
                                if (depth == 0) {
                                    i++; // Skip the closing >
                                    break;
                                }
                            }
                            i++;
                        }
                        i--; // Adjust for loop increment
                    } else {
                        // It's a comparison operator, preserve it
                        sb_append_format(output, "%c", *token.start);
                    }
                }
                break;
                
            case TOKEN_INTERFACE:
                // Skip interface declarations entirely
                i++; // Skip past interface keyword
                while (i < ast->count && ast->tokens[i].type != TOKEN_EOF) {
                    if (ast->tokens[i].type == TOKEN_CODE) {
                        const char *c = ast->tokens[i].start;
                        if (*c == '{') {
                            // Skip to matching }
                            int brace_count = 1;
                            i++;
                            while (i < ast->count && brace_count > 0) {
                                if (ast->tokens[i].type == TOKEN_CODE) {
                                    if (*ast->tokens[i].start == '{') brace_count++;
                                    else if (*ast->tokens[i].start == '}') brace_count--;
                                }
                                i++;
                            }
                            break;
                        } else if (*c == '\n') {
                            break;
                        }
                    }
                    i++;
                }
                i--; // Adjust for loop increment
                break;
                
            case TOKEN_TYPE:
                // Skip type alias declarations: type X = Y;
                i++; // Skip past 'type' keyword
                while (i < ast->count && ast->tokens[i].type != TOKEN_EOF) {
                    Token t = ast->tokens[i];
                    if (t.type == TOKEN_CODE) {
                        char c = *t.start;
                        if (c == ';' || c == '\n') {
                            if (c == ';') i++;
                            break;
                        }
                    }
                    i++;
                }
                i--;
                break;
                
            case TOKEN_COLON:
                // Skip type annotation after colon
                i++;
                int depth = 0;
                while (i < ast->count) {
                    Token t = ast->tokens[i];
                    
                    // Track nesting depth for complex types
                    if (t.type == TOKEN_LT) depth++;
                    else if (t.type == TOKEN_GT) {
                        if (depth > 0) depth--;
                    }
                    
                    // Stop at delimiter when not nested
                    if (depth == 0 && t.type == TOKEN_CODE) {
                        char c = *t.start;
                        if (c == ',' || c == ';' || c == '{' || c == '}' || c == '\n' || c == ')') {
                            i--;
                            break;
                        }
                    }
                    
                    // Also stop at = when not nested (for variable declarations)
                    if (depth == 0 && t.type == TOKEN_EQ) {
                        i--;
                        break;
                    }
                    
                    i++;
                }
                break;
                
            case TOKEN_IMPLEMENTS:  {
                sb_append(output, " ");
                i++; // Skip 'implements' tokense
                sb_append(output, " ");
                while (i < ast->count && ast->tokens[i].type != TOKEN_EOF) {
                    if (ast->tokens[i].type == TOKEN_CODE && *ast->tokens[i].start == '{') {
                        i--;
                        break;
                    }
                    i++;
                }
                break;
                
            case TOKEN_AS:
                // Skip as type assertion
                sb_append(output, " ");
                i++;
                while (i < ast->count) {
                    if (ast->tokens[i].type == TOKEN_CODE) {
                        char c = *ast->tokens[i].start;
                        if (!isalnum(c) && c != '_' && c != '.' && c != '<' && c != '>') {
                            i--;
                            break;
                        }
                    }
                    i++;
                }
                break;
                
            case TOKEN_OPTIONAL:
                // Skip ?: but keep :
                sb_append(output, ":");
                break;
                
            case TOKEN_PRIVATE:
                // Skip private keyword
                break;
                
            case TOKEN_EOF:
                break;
        }
    }
}
    
    return sb_to_string(output);
}

// ============================================================================
// High-level API
// ============================================================================

char* strip_types(const char *source, size_t size) {
    AST *ast = lex(source, size);
    if (!ast) {
        return NULL;
    }
    
    char *result = parse(ast, source);
    ast_free(ast);
    return result;
}

void ast_free(AST *ast) {
    if (ast) {
        free(ast->tokens);
        free(ast);
    }
}
