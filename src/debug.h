#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG

#define debug_print(fmt, ...) \
        do { dbg_print("%s:%d:%s(): " fmt "\n", __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define debug_init() dbg_init()
#define debug_shutdown() dbg_shutdown()

void dbg_print(const char *fmt, ...);
void dbg_init();
void dbg_shutdown();

# else 

#define debug_print(fmt, ...)
#define debug_init 
#define debug_shutdown 

#endif

#endif