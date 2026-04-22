#include "ft_malloc.h"

/*
 * Converts a chunk header pointer to a user payload pointer.
 */
void *chunk_to_payload(const t_chunk *const chunk)
{
    if (!chunk)
        return (NULL);
    return (OFFSET_PTR(void, chunk, CHUNK_HEADER_SIZE));
}

/*
 * Converts a user payload pointer to its chunk header pointer.
 */
t_chunk *payload_to_chunk(const void *const ptr)
{
    if (!ptr)
        return (NULL);
    return (OFFSET_PTR(t_chunk, ptr, -(long) CHUNK_HEADER_SIZE));
}

/*
 * Calculates the next contiguous chunk in memory.
 * Returns NULL if we hit the zone boundary.
 */
t_chunk *chunk_get_next(const t_chunk *const chunk, const t_zone *const zone)
{
    if (!chunk || !zone)
        return (NULL);

    t_chunk *const next = OFFSET_PTR(t_chunk, chunk, CHUNK_SIZE(chunk));
    if ((const void *) next >= OFFSET_PTR(const void, zone, zone->size))
        return (NULL);
    return (next);
}

/*
 * Uses the prev_size field to jump backwards to the previous chunk in O(1) time.
 * Only valid if the previous chunk is actually free.
 */
t_chunk *chunk_get_prev(const t_chunk *const chunk)
{
    if (!chunk || !IS_PREV_FREE(chunk))
        return (NULL);
    return (OFFSET_PTR(t_chunk, chunk, -(long) chunk->prev_size));
}

/*
 * Validates a chunk's size against corruption.
 */
bool is_valid_chunk(const t_chunk *const c, const t_zone *const z)
{
    (void) z;
    if (!c)
        return (false);
    return (CHUNK_SIZE(c) >= MIN_CHUNK_SIZE);
}

/*
 * Scans a zone linearly for the first free chunk large enough for the request.
 */
t_chunk *chunk_find_free(const t_zone *const zone, const size_t needed_size)
{
    if (!zone)
        return (NULL);

    const t_chunk *c = OFFSET_PTR(t_chunk, zone, ZONE_HEADER_SIZE);

    while ((const void *) c < OFFSET_PTR(const void, zone, zone->size) && is_valid_chunk(c, zone)) {
        if (IS_FREE(c) && CHUNK_SIZE(c) >= needed_size)
            return ((t_chunk *) c);
        c = chunk_get_next(c, zone);
        if (!c)
            break;
    }
    return (NULL);
}

/*
 * Checks if the chunk can be split without creating a too-small chunk.
 */
bool can_split_chunk(const size_t total_size, const size_t needed_size)
{
    return (total_size >= needed_size + MIN_CHUNK_SIZE);
}

/*
 * Splits a free chunk if it is significantly larger than needed.
 * Updates metadata for the newly allocated chunk, the remainder free chunk,
 * and the subsequent chunk's prev_size marker.
 */
t_chunk *chunk_split(t_zone *const zone, t_chunk *const chunk, const size_t needed_size)
{
    if (!zone || !chunk)
        return (NULL);

    const size_t total_size = CHUNK_SIZE(chunk);

    if (!can_split_chunk(total_size, needed_size)) {
        SET_ALLOCATED(chunk);
        t_chunk *const next = chunk_get_next(chunk, zone);
        if (next)
            SET_PREV_ALLOC(next);
        return (chunk);
    }

    t_chunk *const remainder = OFFSET_PTR(t_chunk, chunk, needed_size);
    remainder->size          = (total_size - needed_size) | FLAG_FREE;
    SET_PREV_ALLOC(remainder);

    const bool was_prev_free = IS_PREV_FREE(chunk);
    chunk->size              = needed_size;
    if (was_prev_free)
        SET_PREV_FREE(chunk);
    SET_ALLOCATED(chunk);

    t_chunk *const next = chunk_get_next(remainder, zone);
    if (next) {
        next->prev_size = CHUNK_SIZE(remainder);
        SET_PREV_FREE(next);
    }

    return (chunk);
}

/*
 * Coalesces a free chunk with its contiguous neighbors (both forward and backward)
 * to aggressively defragment the heap in O(1) time.
 */
t_chunk *chunk_coalesce(t_zone *const zone, t_chunk *chunk)
{
    if (!zone || !chunk)
        return (NULL);

    // Coalesce forward
    t_chunk *const next = chunk_get_next(chunk, zone);
    if (next && IS_FREE(next)) {
        chunk->size += CHUNK_SIZE(next);
        t_chunk *const next_next = chunk_get_next(chunk, zone);
        if (next_next) {
            next_next->prev_size = CHUNK_SIZE(chunk);
            SET_PREV_FREE(next_next);
        }
    }

    // Coalesce backward
    if (IS_PREV_FREE(chunk)) {
        t_chunk *const prev = chunk_get_prev(chunk);
        if (prev) {
            prev->size += CHUNK_SIZE(chunk);
            t_chunk *const next_to_chunk = chunk_get_next(prev, zone);
            if (next_to_chunk) {
                next_to_chunk->prev_size = CHUNK_SIZE(prev);
                SET_PREV_FREE(next_to_chunk);
            }
            chunk = prev;
        }
    }
    return (chunk);
}
