#include "ft_malloc.h"

/*
 * Internal routine for freeing a verified chunk within a TINY or SMALL zone.
 * It marks the chunk as free, updates the adjacent next chunk's prev_size,
 * executes an O(1) coalesce on both ends, and aggressively returns the zone
 * to the OS if it becomes completely monolithic and empty.
 */
static void free_in_zone(t_zone **list, t_zone *const z, t_chunk *c)
{
    if (!list || !z || !c)
        return;

    // Mark this chunk as free
    SET_FREE(c);

    // Notify the physically next chunk that its previous neighbor is free
    t_chunk *const next = chunk_get_next(c, z);
    if (next) {
        next->prev_size = CHUNK_SIZE(c);
        SET_PREV_FREE(next);
    }

    // Perform aggressive coalescing with adjacent free chunks
    c = chunk_coalesce(z, c);

    // If the chunk now spans the entire workable area of the zone,
    // the zone is entirely empty and should be unmapped to save memory.
    if (CHUNK_SIZE(c) == z->size - ZONE_HEADER_SIZE) {
        zone_remove(list, z);
        sys_munmap(z, z->size);
    }
}

/*
 * Public API wrapper. Ensures the passed pointer is not garbage
 * by validating it against the linked lists (O(N) operation) before
 * attempting to mutate chunk metadata.
 */
void ft_free(void *ptr)
{
    if (!ptr)
        return;

    arena_lock();

    t_zone  *z = NULL;
    t_chunk *c = NULL;

    if (zone_contains_ptr(g_arena.tiny_zones, ptr, &z, &c)) {
        free_in_zone(&g_arena.tiny_zones, z, c);
    } else if (zone_contains_ptr(g_arena.small_zones, ptr, &z, &c)) {
        free_in_zone(&g_arena.small_zones, z, c);
    } else if (zone_contains_ptr(g_arena.large_zones, ptr, &z, &c)) {
        // Large chunks perfectly fit their zones. If we free the chunk,
        // we unmap the entire mapping wrapper.
        zone_remove(&g_arena.large_zones, z);
        sys_munmap(z, z->size);
    }
    // If pointer is NOT in any zone, we intentionally do nothing (no segfault)

    arena_unlock();
}

/*
 * libc interceptor
 */
#ifndef TEST_MODE
void free(void *ptr)
{
    ft_free(ptr);
}
#endif
