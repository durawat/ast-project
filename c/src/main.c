#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyzer/analyzer.h"

#define MAX_FILE_SIZE 1024 * 1024  // 1MB max file size

// Simple argument parser (cross-platform, no getopt dependency)
typedef struct {
    char *file;
    char *output;
    int use_stdin;
    int show_help;
} Args;

int parse_args(int argc, char *argv[], Args *args) {
    args->file = NULL;
    args->output = NULL;
    args->use_stdin = 0;
    args->show_help = 0;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) && i + 1 < argc) {
            args->file = argv[++i];
        } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
            args->output = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stdin") == 0) {
            args->use_stdin = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            args->show_help = 1;
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", program_name);
    fprintf(stderr, "TypeScript/Flow type stripper - converts TypeScript to JavaScript\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -f, --file FILE      Path to the TypeScript file to process\n");
    fprintf(stderr, "  -o, --output FILE    Path to write the output (defaults to same as input)\n");
    fprintf(stderr, "  -s, --stdin          Read code from stdin instead of file\n");
    fprintf(stderr, "  -h, --help           Display this help message\n");
}

char* read_file(const char *filepath, size_t *size) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (*size > MAX_FILE_SIZE) {
        fprintf(stderr, "Error: File too large (max %d bytes)\n", MAX_FILE_SIZE);
        fclose(file);
        return NULL;
    }

    char *content = malloc(*size + 1);
    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(content, 1, *size, file);
    content[read_size] = '\0';
    *size = read_size;

    fclose(file);
    return content;
}

char* read_stdin(size_t *size) {
    size_t capacity = 4096;
    size_t length = 0;
    char *content = malloc(capacity);
    
    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    size_t chunk_size;
    while ((chunk_size = fread(content + length, 1, capacity - length, stdin)) > 0) {
        length += chunk_size;
        if (length >= capacity - 1) {
            capacity *= 2;
            if (capacity > MAX_FILE_SIZE) {
                fprintf(stderr, "Error: Input too large (max %d bytes)\n", MAX_FILE_SIZE);
                free(content);
                return NULL;
            }
            char *new_content = realloc(content, capacity);
            if (!new_content) {
                fprintf(stderr, "Error: Memory reallocation failed\n");
                free(content);
                return NULL;
            }
            content = new_content;
        }
    }

    content[length] = '\0';
    *size = length;
    return content;
}

int write_output(const char *filepath, const char *content) {
    if (strcmp(filepath, "-") == 0) {
        // Write to stdout
        printf("%s", content);
        return 0;
    }

    FILE *file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s' for writing\n", filepath);
        return -1;
    }

    fprintf(file, "%s", content);
    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    Args args;
    
    if (parse_args(argc, argv, &args) != 0) {
        print_usage(argv[0]);
        return 1;
    }

    if (args.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    // Validate input options
    if (!args.use_stdin && !args.file) {
        fprintf(stderr, "Error: Must specify either -f/--file or -s/--stdin\n\n");
        print_usage(argv[0]);
        return 1;
    }

    char *input_file = args.file;
    char *output_file = args.output;
    int use_stdin = args.use_stdin;

    // Read input
    size_t input_size = 0;
    char *code = NULL;

    if (use_stdin) {
        code = read_stdin(&input_size);
        if (!output_file) {
            output_file = "-";  // Default to stdout for stdin input
        }
    } else {
        code = read_file(input_file, &input_size);
        if (!output_file) {
            output_file = input_file;  // Default to overwriting input file
        }
    }

    if (!code) {
        return 1;
    }

    // Strip TypeScript types
    char *result = strip_types(code, input_size);
    free(code);

    if (!result) {
        fprintf(stderr, "Error: Type stripping failed\n");
        return 1;
    }

    // Write output
    int result_code = write_output(output_file, result);
    free(result);

    if (result_code == 0 && strcmp(output_file, "-") != 0) {
        printf("Type stripping complete. Output written to: %s\n", output_file);
    }

    return result_code == 0 ? 0 : 1;
}
