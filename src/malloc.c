#include "ft_malloc.h"

/*
 * Internal logic for fulfilling requests from TINY and SMALL zones.
 * Scans the appropriate zone list for a free chunk. If none is found,
 * it creates a new zone and appends it to the list.
 */
static void *allocate_in_zone(t_zone **zone_list, const size_t needed_size, const size_t zone_size)
{
    t_zone  *z     = *zone_list;
    t_chunk *chunk = NULL;

    while (z) {
        chunk = chunk_find_free(z, needed_size);
        if (chunk)
            break;
        z = z->next;
    }

    if (!chunk) {
        z = zone_create(zone_size);
        if (!z)
            return (NULL);
        zone_append(zone_list, z);
        chunk = chunk_find_free(z, needed_size);
    }

    if (!chunk)
        return (NULL);

    chunk = chunk_split(z, chunk, needed_size);
    return (chunk_to_payload(chunk));
}

/*
 * Internal logic for fulfilling requests for LARGE allocations.
 * Large allocations do not reside in shared zones; each gets its own mmap.
 */
static void *allocate_large(const size_t needed_size)
{
    // Ensure the entire mapping request remains page-aligned and strictly sufficient
    const size_t page_size  = get_page_size();
    const size_t min_req    = ZONE_HEADER_SIZE + needed_size;
    const size_t alloc_size = PAGE_ALIGN(min_req, page_size);

    t_zone *const z = zone_create(alloc_size);
    if (!z)
        return (NULL);

    zone_append(&g_arena.large_zones, z);

    t_chunk *const chunk = OFFSET_PTR(t_chunk, z, ZONE_HEADER_SIZE);

    // It's a precise fit (monolithic), mark it as used immediately
    const bool was_prev_free = IS_PREV_FREE(chunk);
    chunk->size              = (alloc_size - ZONE_HEADER_SIZE);
    if (was_prev_free)
        SET_PREV_FREE(chunk);
    SET_ALLOCATED(chunk);

    return (chunk_to_payload(chunk));
}

/*
 * Public API wrapper. Routes the request based on size constraints.
 */
void *ft_malloc(size_t size)
{
    if (size == 0)
        return (NULL);

    size_t needed_size = ALIGN(size + CHUNK_HEADER_SIZE);
    if (needed_size < MIN_CHUNK_SIZE)
        needed_size = MIN_CHUNK_SIZE;

    void *payload = NULL;

    arena_lock();

    if (size <= TINY_MAX) {
        payload = allocate_in_zone(&g_arena.tiny_zones, needed_size, calculate_zone_size(TINY_MAX));
    } else if (size <= SMALL_MAX) {
        payload =
            allocate_in_zone(&g_arena.small_zones, needed_size, calculate_zone_size(SMALL_MAX));
    } else {
        payload = allocate_large(needed_size);
    }

    arena_unlock();
    return (payload);
}

/*
 * libc interceptor.
 */
#ifndef TEST_MODE
void *malloc(size_t size)
{
    return (ft_malloc(size));
}
#endif
