# AST Project - TypeScript Type Stripper

A lightweight C-based parser that strips TypeScript/Flow type annotations from JavaScript code, converting TypeScript to plain JavaScript.

## Features

- Parse TypeScript/Flow-annotated JavaScript code
- Strip type annotations (`: type`, `interface`, `type`, generics, etc.)
- Read from files or stdin (for selected text)
- Write clean JavaScript to files or stdout
- State machine-based parser for robust handling
- Preserves comments, strings, and code structure

## Architecture

This parser demonstrates fundamental parsing principles using a **two-stage architecture**:

### Stage 1: Lexer

The lexer (`lex()` function) tokenizes source code into an Abstract Syntax Tree (AST):

- **Input**: Raw TypeScript/Flow source code
- **Output**: Array of tokens with type, position, and line information
- **Token Types**:
  - Code tokens: `TOKEN_CODE`, `TOKEN_STRING`, `TOKEN_BLOCK_COMMENT`, `TOKEN_LINE_COMMENT`
  - TypeScript constructs: `TOKEN_INTERFACE`, `TOKEN_TYPE`, `TOKEN_IMPLEMENTS`, `TOKEN_AS`, `TOKEN_PRIVATE`
  - Type patterns: `TOKEN_COLON`, `TOKEN_GENERIC_START/END`, `TOKEN_OPTIONAL`

### Stage 2: Parser

The parser (`parse()` function) processes the token stream to strip types:

- **Input**: AST from lexer
- **Output**: Plain JavaScript string
- **Logic**: Iterates through tokens, preserving code/strings/comments while skipping type-related tokens

### Benefits of Two-Stage Design

- **Separation of Concerns**: Lexer handles character→token, parser handles token→output
- **Debuggability**: Inspect AST between stages
- **Extensibility**: Modify lexer or parser independently
- **Reusability**: AST can be used for other purposes (analysis, transformation, etc.)

### Core Parsing Principles

The architecture demonstrates three fundamental components of any parser:

1. **Input Buffer** - Source code
2. **Output Buffer** - Transformed result
3. **State Machine** - Context tracking (lexer state enum)

## Installation

### Using Make (Linux, macOS, Windows with MinGW/MSYS)

```bash
cd ast-project/c
make
```

### Manual Compilation

```bash
cd ast-project/c
gcc -Wall -Wextra -std=c11 -O2 -o ast-analyzer src/main.c src/analyzer/analyzer.c
```

## Usage

### Strip types from a TypeScript file

```bash
./ast-analyzer -f test/example.ts -o test/example.js
```

### Strip types from stdin to stdout

```bash
cat test/example.ts | ./ast-analyzer -s
```

### Strip types from selected text and write to a file

```bash
cat test/example.ts | ./ast-analyzer -s -o output.js
```

## Command-Line Options

- `-f, --file FILE` - Path to the TypeScript file to process
- `-o, --output FILE` - Path to write the output (defaults to same as input file, or stdout for stdin)
- `-s, --stdin` - Read code from stdin instead of file
- `-h, --help` - Display help message

## Project Structure

```
ast-project/
├── c/                   # C implementation
│   ├── src/
│   │   ├── main.c       # Entry point and CLI handling
│   │   └── analyzer/
│   │       ├── analyzer.h   # Analyzer interface (lex, parse, strip_types APIs)
│   │       └── analyzer.c   # Lexer and parser implementation
│   ├── test/
│   │   ├── example.ts       # Example TypeScript file for testing
│   │   └── build/           # Output directory for generated JavaScript
│   │       └── example.js   # Generated JavaScript output
│   └── Makefile         # Build configuration
├── README.md            # This file
└── .gitignore          # Git ignore rules
```

## How It Works

The type stripper uses a **two-stage lexer+parser architecture**:

### Stage 1: Lexical Analysis (Lexer)

The lexer tokenizes source code character-by-character into an Abstract Syntax Tree (AST):

1. **State Machine**: Tracks context with four lexer states:

   - `STATE_CODE` - Normal code processing
   - `STATE_STRING` - Inside string literals (preserve as single token)
   - `STATE_BLOCK_COMMENT` - Inside `/* */` comments (preserve)
   - `STATE_LINE_COMMENT` - Inside `//` comments (preserve)

2. **Token Recognition**: Identifies TypeScript constructs:

   - Keywords: `interface`, `type`, `implements`, `as`, `private`
   - Patterns: Type annotations (`:`), generics (`<>`), optional parameters (`?:`)
   - Regular code, strings, and comments

3. **AST Output**: Array of tokens, each containing:
   - Token type (enum)
   - Pointer to start position in source
   - Length of token
   - Line number

### Stage 2: Parsing (Type Stripping)

The parser processes the token stream to generate clean JavaScript:

1. **Token Iteration**: Loops through AST tokens
2. **Selective Preservation**:
   - **Preserve**: `TOKEN_CODE`, `TOKEN_STRING`, `TOKEN_BLOCK_COMMENT`, `TOKEN_LINE_COMMENT`
   - **Skip/Remove**: Type-related tokens (interface, type annotations, generics, etc.)
3. **Smart Removal**:
   - Interface declarations: Skip to matching brace or newline
   - Type annotations: Skip from `:` to delimiter (`,`, `;`, `=`, etc.)
   - Generics: Skip `<T>` patterns
   - `implements` clauses: Skip to `{`
   - `as` assertions: Skip type expression
   - `private` keyword: Remove entirely
4. **Output Building**: Concatenates preserved tokens into output buffer

### High-Level API

```c
char* strip_types(const char *source, size_t size) {
    AST *ast = lex(source, size);      // Stage 1: Tokenize
    char *result = parse(ast, source); // Stage 2: Strip types
    ast_free(ast);                     // Cleanupbuild/example.js
- `make clean` - Remove build artifacts
- `make help` - Show available targets

## Development

To extend the type stripper:

### Adding New Token Types
1. Add token type to `TokenType` enum in [analyzer.h](c/src/analyzer/analyzer.h)
2. Implement recognition logic in `lex()` function in [analyzer.c](c/src/analyzer/analyzer.c)
3. Add handling logic in `parse()` function

### Modifying Stripping Behavior
1. Locate the token case in `parse()` switch statement
2. Modify the skipping/preservation logic
3. Test with [c/test/example.ts](c/test/example.ts)

### Architecture Benefits
The **two-stage lexer+parser** design provides:
- **Debuggability**: Inspect AST tokens between stages
- **Modularity**: Modify lexer or parser independently
- **Testability**: Test tokenization and transformation separately
- **Reusability**: Use the lexer for other AST-based tools

Rebuild with `make` in the `c/` directory after changes
```

The stripper produces pure JavaScript:

```javascript
function getUser(id) {
  return { id: 1, name: "John" };
}
```

## Make Targets

- `make` or `make all` - Build the project
- `make test` - Run stripper on test/example.ts to produce test/example.js
- `make clean` - Remove build artifacts
- `make help` - Show available targets

## Development

To extend the type stripper:

1. Modify [c/src/analyzer/analyzer.c](c/src/analyzer/analyzer.c) to handle additional TypeScript features
2. Update `process_code_token()` function for new type patterns
3. Add test cases to [c/test/example.ts](c/test/example.ts)
4. Rebuild with `make` in the `c/` directory

The parser follows a simple architecture principle: **three core variables** drive all parsing:

- Input buffer (source code)
- Output buffer (result)
- State machine enum (context)

This pattern can be applied to any text transformation or parsing task.

## Multi-Language Support

This project is structured to support implementations in multiple languages. The C implementation is in the `c/` directory. Additional language implementations can be added in separate directories (e.g., `python/`, `rust/`, etc.).

## License

MIT
