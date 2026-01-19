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

This parser demonstrates fundamental parsing principles using three core components:

1. **Input Buffer** - Source code to be parsed
2. **Output Buffer** - Resulting JavaScript without type annotations
3. **State Machine** (enum-based) - Tracks parser context (code, string, comment states)

These three variables form the foundation of any lexical parser, enabling context-aware character processing.

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
│   │       ├── analyzer.h   # Analyzer interface
│   │       └── analyzer.c   # Type stripper with state machine parser
│   ├── test/
│   │   ├── example.ts   # Example TypeScript file for testing
│   │   └── example.c    # Example C file
│   └── Makefile         # Build configuration
├── README.md            # This file
└── .gitignore          # Git ignore rules
```

## How It Works

The type stripper uses a **state machine parser** with the following approach:

1. **Input Buffer**: Reads TypeScript/Flow source code into memory
2. **State Machine**: Tracks parser context with four states:
   - `STATE_CODE` - Normal code processing
   - `STATE_STRING` - Inside string literals (preserve content)
   - `STATE_BLOCK_COMMENT` - Inside `/* */` comments (preserve)
   - `STATE_LINE_COMMENT` - Inside `//` comments (preserve)
3. **Output Buffer**: Dynamically builds the stripped JavaScript output
4. **Type Detection**: In CODE state, identifies and removes:
   - Type annotations (`: type`)
   - Interface declarations (`interface X { }`)
   - Type aliases (`type X = Y`)
   - Generics (`<T>`)
   - `implements` clauses
   - `as` type assertions
   - Optional parameter markers (`?:`)

The parser reads character-by-character, switching states based on context to ensure types are only stripped from actual code, not from strings or comments.

## Example Output

Given [example.ts](c/test/example.ts) with TypeScript:

```typescript
interface User {
  id: number;
  name: string;
}

function getUser(id: number): User {
  return { id: 1, name: "John" };
}
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
