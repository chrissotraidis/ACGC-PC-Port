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

/* Creates and enters Bellpad's Application Support directory. A development
 * run may set BELLPAD_DATA_HOME to an isolated absolute path. */
int pc_apple_prepare_data_directory(void);

/* Remembers the image the player chose so later launches do not ask again.
 * The reference is stored inside the prepared data directory as an alias, so
 * a renamed or moved image still resolves. No retail data is copied. */
int pc_apple_store_disc_path(const char* path);

/* Resolves the remembered image without presenting UI. Returns 0 when nothing
 * is remembered or the reference no longer resolves. */
int pc_apple_load_disc_path(char* out_path, size_t capacity);

/* Drops the remembered reference. */
void pc_apple_forget_disc_path(void);

#ifdef __cplusplus
}
#endif

#endif
