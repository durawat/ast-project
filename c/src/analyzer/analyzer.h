#ifndef ANALYZER_H
#define ANALYZER_H

#include <stddef.h>

// Strip TypeScript/Flow type annotations from JavaScript code
// The caller is responsible for freeing the returned string
char* strip_types(const char *source, size_t size);



#endif // ANALYZER_H
