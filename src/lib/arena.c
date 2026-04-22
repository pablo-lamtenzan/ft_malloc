#include "ft_malloc.h"

/*
 * Global memory state instance.
 * Thread-safety provided by static mutex initialization.
 */
t_malloc_state g_arena = {.lock        = PTHREAD_MUTEX_INITIALIZER,
                          .tiny_zones  = NULL,
                          .small_zones = NULL,
                          .large_zones = NULL};

/*
 * Lock the global allocator state.
 */
void arena_lock(void)
{
    pthread_mutex_lock(&g_arena.lock);
}

/*
 * Unlock the global allocator state.
 */
void arena_unlock(void)
{
    pthread_mutex_unlock(&g_arena.lock);
}
