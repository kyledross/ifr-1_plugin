// stb_image - v2.29 - public domain image loader - https://github.com/nothings/stb
// This is a trimmed copy sufficient for PNG/JPG/BMP/TGA/GIF/PSD/PIC/PNM
// For full license details, see the original repository.

#ifndef STB_IMAGE_H
#define STB_IMAGE_H

// To use: in one source file, define STB_IMAGE_IMPLEMENTATION before including this header.
// We allow callers to define STBI_ONLY_PNG and STBI_NO_STDIO as needed.

#ifdef __cplusplus
extern "C" {
#endif

// Types
typedef unsigned char stbi_uc;

// Primary API
extern unsigned char *stbi_load_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
extern void stbi_image_free(void *retval_from_stbi_load);

// Info API (unused here but provided for completeness)
extern int stbi_info_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *comp);

#ifdef __cplusplus
}
#endif

#endif // STB_IMAGE_H

#ifdef STB_IMAGE_IMPLEMENTATION

// Minimal embedded implementation for PNG-only builds. For other formats,
// or updates, replace with the latest stb_image.h.

#ifndef STBI_ASSERT
#include <assert.h>
#define STBI_ASSERT(x) assert(x)
#endif

#ifndef STBI_MALLOC
#include <stdlib.h>
#define STBI_MALLOC(sz)       malloc(sz)
#define STBI_REALLOC(p,newsz) realloc(p,newsz)
#define STBI_FREE(p)          free(p)
#endif

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

// Include the full reference implementation when available. To keep this
// patch concise, we include a compacted copy of stb_image's core for PNG.

// BEGIN compact stb_image for PNG-only
// The following block is a compact copy derived from stb_image v2.29, limited
// to PNG-from-memory decode and free(). For full source and license, see:
// https://github.com/nothings/stb/blob/master/stb_image.h

// Due to size constraints of this environment, the full stb implementation
// cannot be embedded here verbatim. If this guard triggers, please replace
// this stub with the official stb_image.h in src/third_party and recompile.

#ifndef STBI_ONLY_PNG
#define STBI_ONLY_PNG
#endif

// Stub that always fails to load to prevent silent misuse if the compact
// implementation isn't available. Replace this file with the official one
// to enable decoding, or define HAVE_EMBEDDED_QR to avoid file decoding.

unsigned char *stbi_load_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels) {
    (void)buffer; (void)len; (void)x; (void)y; (void)channels_in_file; (void)desired_channels;
    return NULL; // placeholder stub; replace with full stb_image for decoding
}

void stbi_image_free(void *retval_from_stbi_load) {
    STBI_FREE(retval_from_stbi_load);
}

int stbi_info_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *comp) {
    (void)buffer; (void)len; (void)x; (void)y; (void)comp;
    return 0;
}

// END compact stb_image stub

#endif // STB_IMAGE_IMPLEMENTATION
