#ifndef _DEBUG_H
#define _DEBUG_H

void debug_pin_toggle(uint8_t pin_num);
void debug_failover_log(uint32_t level, const char *fmt, ...);
void debug_failed_malloc(const char *file, int line);

// Safe logging (always works, even when heap is dead)
/*
#define LOG_FAILOVER(level, fmt, ...) \
    debug_failover_log(level, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
*/
#define LOG_FAILOVER(level, fmt, ...) \
    printf("%s [%s:%d] " fmt, level, __FILE__, __LINE__, ##__VA_ARGS__)
// One-liner to wrap every pvPortMalloc()
#define ASSERT_MALLOC(ptr) \
    do { if ((ptr) == NULL) debug_failed_malloc(__FILE__, __LINE__); } while(0)

#endif
