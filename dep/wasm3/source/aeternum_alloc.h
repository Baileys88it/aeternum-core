#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void* aeternum_wasm_calloc(size_t n, size_t s);
void* aeternum_wasm_realloc(void* p, size_t s);
void aeternum_wasm_free(void* p);
#ifdef __cplusplus
}
#endif
