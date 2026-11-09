#ifndef IMPORT_MODULE
#define IMPORT_MODULE "aeternum"
#endif
#ifndef IMPORT_NAME
#define IMPORT_NAME "aeternum_host_log"
#endif
#ifndef EXPORT_NAME
#define EXPORT_NAME "on_init"
#endif
__attribute__((import_module(IMPORT_MODULE), import_name(IMPORT_NAME)))
#ifdef BAD_SIGNATURE
extern long long aeternum_host_log(const char*);
#else
extern void aeternum_host_log(const char*);
#endif
#ifdef CALL_LOOP
__attribute__((noinline)) static void step(void) { static volatile unsigned n; ++n; }
#endif
__attribute__((export_name(EXPORT_NAME))) void on_init(
#ifdef BAD_ENTRY_SIGNATURE
int ignored
#else
void
#endif
) {
#if defined(INFINITE_LOOP)
 aeternum_host_log("fuel loop entered");
 for(;;) __asm__ volatile("");
#elif defined(CALL_LOOP)
 aeternum_host_log("fuel call loop entered");
 for(;;) step();
#elif defined(FINITE_LOOP)
 volatile unsigned n=0;for(unsigned i=0;i<100;++i)++n;
 aeternum_host_log("finite loop passed");
#elif defined(BAD_POINTER)
 aeternum_host_log((const char*)0xfffffffcU);
#elif defined(LAST_BYTE)
 volatile char* last=(volatile char*)131071U;*last='X';aeternum_host_log((const char*)last);
#elif defined(NULL_POINTER)
 aeternum_host_log((const char*)0);
#elif defined(UNTERMINATED)
 static char text[256];for(unsigned i=0;i<sizeof text;++i)text[i]='X';aeternum_host_log(text);
#elif defined(FORMAT_DATA)
 aeternum_host_log("%s%n{}");
#elif defined(MEMORY_GROW)
 aeternum_host_log(__builtin_wasm_memory_grow(0,16)==(unsigned long)-1 ? "memory.grow rejected" : "unexpected growth");
#else
 aeternum_host_log("WASM Sandbox Operativa");
#endif
}
