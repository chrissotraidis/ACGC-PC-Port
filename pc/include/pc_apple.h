#ifndef PC_APPLE_H
#define PC_APPLE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Presents a native macOS picker for a user-owned disc image. */
int pc_apple_choose_disc(char* out_path, size_t capacity);

/* Resolves a file packaged in Bellpad.app without changing the process cwd. */
int pc_apple_resource_path(const char* relative_path, char* out_path,
                           size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
