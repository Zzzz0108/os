#ifndef PLATFORM_THREAD_H
#define PLATFORM_THREAD_H

typedef unsigned int (*os_thread_func_t)(void*);
typedef void* os_thread_t;

/* Create a new thread and return an opaque handle. */
os_thread_t os_thread_create(os_thread_func_t func, void* arg);

/* Wait until thread finishes execution. */
int os_thread_join(os_thread_t thread);

/* Release thread handle resources. */
void os_thread_close(os_thread_t thread);

#endif
