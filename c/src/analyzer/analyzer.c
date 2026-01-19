#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

// Keyword indices
typedef enum {
    KW_INTERFACE,
    KW_TYPE,
    KW_IMPLEMENTS,
    KW_AS,
    KW_PRIVATE,
    KW_COUNT
} KeywordIndex;

// Pattern indices
typedef enum {
    PATTERN_COLON,        // : type annotations
    PATTERN_GENERICS,     // <> generic parameters
    PATTERN_OPTIONAL,     // ?: optional parameters
    PATTERN_COUNT
} PatternIndex;

// TypeScript/Flow keywords to strip
static const char *KEYWORDS_TO_STRIP[] = {
    [KW_INTERFACE] = "interface",
    [KW_TYPE] = "type",
    [KW_IMPLEMENTS] = "implements",
    [KW_AS] = "as",
    [KW_PRIVATE] = "private",
    [KW_COUNT] = NULL
};

// Special characters/patterns to strip
static const char *PATTERNS_TO_STRIP[] = {
    [PATTERN_COLON] = ":",
    [PATTERN_GENERICS] = "<>",
    [PATTERN_OPTIONAL] = "?:",
    [PATTERN_COUNT] = NULL
};

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

// Parser states for TypeScript type stripper
typedef enum {
    STATE_CODE,           // Normal code
    STATE_STRING,         // Inside string literal
    STATE_BLOCK_COMMENT,  // Inside /* */ comment
    STATE_LINE_COMMENT    // Inside // comment
} ParserState;

// Forward declaration
static int process_code_token(const char **ptr, const char *end, const char *source, StringBuilder *output);

// TypeScript/Flow type stripper
char* strip_types(const char *source, size_t size) {
    if (!source || size == 0) {
        return NULL;
    }

    StringBuilder *output = sb_create(size);
    if (!output) {
        return NULL;
    }

    const char *ptr = source;
    const char *end = source + size;
    ParserState state = STATE_CODE;
    char string_delimiter = 0;

    while (ptr < end) {
        char current = *ptr;
        char next = (ptr + 1 < end) ? *(ptr + 1) : '\0';

        // State transitions
        switch (state) {
            case STATE_CODE:
                // Transition to string state
                if ((current == '"' || current == '\'' || current == '`') && 
                    (ptr == source || *(ptr - 1) != '\\')) {
                    state = STATE_STRING;
                    string_delimiter = current;
                    sb_append_format(output, "%c", current);
                    ptr++;
                    continue;
                }
                
                // Transition to block comment
                if (current == '/' && next == '*') {
                    state = STATE_BLOCK_COMMENT;
                    sb_append(output, "/*");
                    ptr += 2;
                    continue;
                }
                
                // Transition to line comment
                if (current == '/' && next == '/') {
                    state = STATE_LINE_COMMENT;
                    sb_append(output, "//");
                    ptr += 2;
                    continue;
                }
                
                // Process code (strip types)
                if (!process_code_token(&ptr, end, source, output)) {
                    sb_append_format(output, "%c", current);
                    ptr++;
                }
                break;

            case STATE_STRING:
                sb_append_format(output, "%c", current);
                // Transition back to code when string ends
                if (current == string_delimiter && (ptr == source || *(ptr - 1) != '\\')) {
                    state = STATE_CODE;
                }
                ptr++;
                break;

            case STATE_BLOCK_COMMENT:
                sb_append_format(output, "%c", current);
                // Transition back to code when comment ends
                if (current == '*' && next == '/') {
                    sb_append_format(output, "%c", next);
                    state = STATE_CODE;
                    ptr += 2;
                    continue;
                }
                ptr++;
                break;

            case STATE_LINE_COMMENT:
                sb_append_format(output, "%c", current);
                // Transition back to code at newline
                if (current == '\n') {
                    state = STATE_CODE;
                }
                ptr++;
                break;
        }
    }

    return sb_to_string(output);
}

// Process a code token and strip TypeScript types
// Returns 1 if token was processed and ptr was advanced, 0 otherwise
static int process_code_token(const char **ptr, const char *end, const char *source, StringBuilder *output) {
    const char *p = *ptr;

    // Skip "interface" declarations
    if (strncmp(p, KEYWORDS_TO_STRIP[KW_INTERFACE], 9) == 0 && (p + 9 >= end || (!isalnum(*(p + 9)) && *(p + 9) != '_'))) {
        int brace_count = 0;
        int found_brace = 0;
        while (p < end) {
            if (*p == '{') {
                brace_count++;
                found_brace = 1;
            } else if (*p == '}') {
                brace_count--;
                if (found_brace && brace_count == 0) {
                    p++;
                    break;
                }
            } else if (*p == '\n' && !found_brace) {
                p++;
                break;
            }
            p++;
        }
        *ptr = p;
        return 1;
    }

    // Skip "type" declarations
    if (strncmp(p, KEYWORDS_TO_STRIP[KW_TYPE], 4) == 0 && (p + 4 < end && *(p + 4) == ' ')) {
        while (p < end && *p != ';' && *p != '\n') {
            p++;
        }
        if (p < end && *p == ';') p++;
        *ptr = p;
        return 1;
    }

    // Strip type annotations after colons (":") 
    if (*p == PATTERNS_TO_STRIP[PATTERN_COLON][0]) {
        const char *check = p - 1;
        while (check >= source && isspace(*check)) check--;
        
        if (check >= source && (isalnum(*check) || *check == '_' || *check == ')' || *check == ']')) {
            p++; // skip colon
            while (p < end && isspace(*p)) p++;
            
            // Skip the type expression
            int angle_brackets = 0, parens = 0, brackets = 0;
            
            while (p < end) {
                if (*p == '<') angle_brackets++;
                else if (*p == '>') {
                    if (angle_brackets > 0) angle_brackets--;
                    else break;
                }
                else if (*p == '(') parens++;
                else if (*p == ')') {
                    if (parens > 0) parens--;
                    else break;
                }
                else if (*p == '[') brackets++;
                else if (*p == ']') {
                    if (brackets > 0) brackets--;
                    else break;
                }
                else if (((*p == ',' || *p == ';' || *p == '{' || *p == '}' || *p == '=' || *p == '\n') && 
                         angle_brackets == 0 && parens == 0 && brackets == 0)) {
                    break;
                }
                p++;
            }
            *ptr = p;
            return 1;
        }
    }

    // Strip generic type parameters ("<>")
    if (*p == PATTERNS_TO_STRIP[PATTERN_GENERICS][0]) {
        const char *check = p - 1;
        while (check >= source && isspace(*check)) check--;
        
        if (check >= source && (isalnum(*check) || *check == '_')) {
            const char *lookahead = p + 1;
            int could_be_generic = 0;
            
            while (lookahead < end && isspace(*lookahead)) lookahead++;
            if (lookahead < end && (isalpha(*lookahead) || *lookahead == '_')) {
                int depth = 1;
                lookahead++;
                while (lookahead < end && depth > 0) {
                    if (*lookahead == '<') depth++;
                    else if (*lookahead == '>') depth--;
                    else if (*lookahead == ';' || *lookahead == '{') break;
                    lookahead++;
                }
                
                if (depth == 0 && lookahead < end) {
                    while (lookahead < end && isspace(*lookahead)) lookahead++;
                    if (lookahead < end && (*lookahead == '(' || *lookahead == '=' || *lookahead == '{' || 
                        (lookahead + 1 < end && *lookahead == '=' && *(lookahead + 1) == '>'))) {
                        could_be_generic = 1;
                    }
                }
            }
            
            if (could_be_generic) {
                int depth = 1;
                p++;
                while (p < end && depth > 0) {
                    if (*p == '<') depth++;
                    else if (*p == '>') {
                        depth--;
                        if (depth == 0) {
                            p++;
                            break;
                        }
                    }
                    p++;
                }
                *ptr = p;
                return 1;
            }
        }
    }

    // Strip "as" type assertions
    if (p + 3 < end && p > source && *(p - 1) == ' ' && 
        strncmp(p, KEYWORDS_TO_STRIP[KW_AS], 2) == 0 && *(p + 2) == ' ') {
        p += 4;
        while (p < end && isspace(*p)) p++;
        
        while (p < end && (isalnum(*p) || *p == '_' || *p == '.' || *p == '<' || *p == '>')) {
            if (*p == '<') {
                int depth = 1;
                p++;
                while (p < end && depth > 0) {
                    if (*p == '<') depth++;
                    else if (*p == '>') depth--;
                    p++;
                }
            } else {
                p++;
            }
        }
        sb_append(output, " ");
        *ptr = p;
        return 1;
    }

    // Strip "implements" clause
    if (p + 10 < end && strncmp(p, KEYWORDS_TO_STRIP[KW_IMPLEMENTS], 10) == 0 && *(p + 10) == ' ') {
        p += 11;
        while (p < end && *p != '{') p++;
        sb_append(output, " ");
        *ptr = p;
        return 1;
    }

    // Remove "?:" optional parameter markers (but keep ternary operators)
    if (*p == PATTERNS_TO_STRIP[PATTERN_OPTIONAL][0] && p + 1 < end && *(p + 1) == PATTERNS_TO_STRIP[PATTERN_OPTIONAL][1]) {
        p++; // skip the ?
        *ptr = p;
        return 1;
    }
// Strip "private" keyword
    if (strncmp(p, KEYWORDS_TO_STRIP[KW_PRIVATE], 7) == 0 && (p + 7 >= end || (!isalnum(*(p + 7)) && *(p + 7) != '_'))) {
        p += 7;
        while (p < end && isspace(*p)) p++;
        *ptr = p;
        return 1;
    }

    
    return 0;
}
