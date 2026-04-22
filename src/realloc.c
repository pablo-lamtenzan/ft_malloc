#include "ft_malloc.h"

/*
 * Self-contained memory copy to avoid external libft dependencies
 * for critical internal logic.
 */
static void ft_memcpy(void *dst, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *) dst;
    const unsigned char *s = (const unsigned char *) src;

    while (n--)
        *d++ = *s++;
}

/*
 * Checks all zone linked lists to verify the pointer.
 * Returns the zone and chunk if valid, else false.
 */
static bool get_chunk_for_realloc(const void *const ptr, t_zone **out_z, t_chunk **out_c)
{
    if (zone_contains_ptr(g_arena.tiny_zones, ptr, out_z, out_c))
        return (true);
    if (zone_contains_ptr(g_arena.small_zones, ptr, out_z, out_c))
        return (true);
    if (zone_contains_ptr(g_arena.large_zones, ptr, out_z, out_c))
        return (true);
    return (false);
}

/*
 * Adjusts the size of an existing allocation if possible,
 * else performs a malloc-copy-free.
 */
void *ft_realloc(void *ptr, size_t size)
{
    if (!ptr)
        return (ft_malloc(size));
    if (size == 0) {
        ft_free(ptr);
        return (NULL);
    }

    size_t needed_size = ALIGN(size + CHUNK_HEADER_SIZE);
    if (needed_size < MIN_CHUNK_SIZE)
        needed_size = MIN_CHUNK_SIZE;

    arena_lock();

    t_zone  *z = NULL;
    t_chunk *c = NULL;

    if (!get_chunk_for_realloc(ptr, &z, &c)) {
        arena_unlock();
        return (NULL); // Invalid pointer
    }

    const size_t old_user_size = CHUNK_SIZE(c) - CHUNK_HEADER_SIZE;

    // Scenario 1: The current chunk is already large enough (shrinking or exact)
    if (needed_size <= CHUNK_SIZE(c)) {
        c = chunk_split(z, c, needed_size);
        arena_unlock();
        return (ptr);
    }

    // Scenario 2: Try coalescing strictly forward to satisfy growth in-place
    t_chunk *const next = chunk_get_next(c, z);
    if (next && IS_FREE(next) && CHUNK_SIZE(c) + CHUNK_SIZE(next) >= needed_size) {
        c->size += CHUNK_SIZE(next);

        // Update the next_next chunk to point to our newly enlarged chunk
        t_chunk *const next_next = chunk_get_next(c, z);
        if (next_next) {
            next_next->prev_size = CHUNK_SIZE(c);
            SET_PREV_FREE(next_next);
        }

        c = chunk_split(z, c, needed_size);
        arena_unlock();
        return (ptr);
    }

    // Scenario 3: In-place growth failed. Must allocate new, copy, and free old.
    arena_unlock();

    void *const new_ptr = ft_malloc(size);
    if (!new_ptr)
        return (NULL);

    ft_memcpy(new_ptr, ptr, old_user_size < size ? old_user_size : size);
    ft_free(ptr);

    return (new_ptr);
}

/*
 * libc interceptor
 */
#ifndef TEST_MODE
void *realloc(void *ptr, size_t size)
{
    return (ft_realloc(ptr, size));
}
#endif
