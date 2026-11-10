wasm3 v0.5.0, MIT, https://github.com/wasm3/wasm3
Pinned commit 6b8bcb1e07bf26ebef09a7211b0a37a446eafd52.
Vendored source and LICENSE from upstream codeload archive; no git clone.
Local delta: m3_core.c routes its three CRT allocator calls through accounted
Aeternum wrappers declared in aeternum_alloc.h. Other sources unchanged.
Only interpreter sources built; no WASI/libc/tracer host bindings linked.
