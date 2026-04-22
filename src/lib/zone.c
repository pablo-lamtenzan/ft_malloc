#include "ft_malloc.h"

/*
 * Calculates the required zone size (page aligned) to hold at least
 * MIN_ALLOCS_PER_ZONE allocations of max_alloc_size.
 */
size_t calculate_zone_size(const size_t max_alloc_size)
{
    const size_t page_size = get_page_size();
    const size_t min_req =
        ZONE_HEADER_SIZE + (MIN_ALLOCS_PER_ZONE * CHUNK_TOTAL_SIZE(max_alloc_size));

    return (PAGE_ALIGN(min_req, page_size));
}

/*
 * Creates a new zone by mmap'ing alloc_size bytes.
 * Initializes the first monolithic free chunk.
 */
t_zone *zone_create(const size_t alloc_size)
{
    if (alloc_size == 0)
        return (NULL);

    t_zone *const z = (t_zone *) sys_mmap(alloc_size);
    if (!z)
        return (NULL);

    z->size = alloc_size;
    z->next = NULL;
    z->prev = NULL;

    t_chunk *const first = OFFSET_PTR(t_chunk, z, ZONE_HEADER_SIZE);
    first->prev_size     = 0;

    // The entire zone excluding the header is one big free chunk.
    first->size = (alloc_size - ZONE_HEADER_SIZE);
    SET_FREE(first);
    // Explicitly clearing PREV_FREE as there is no previous chunk
    SET_PREV_ALLOC(first);

    return (z);
}

/*
 * Appends a new zone to the end of a zone list.
 */
void zone_append(t_zone **list, t_zone *const new_zone)
{
    if (!list || !new_zone)
        return;

    if (!*list) {
        *list = new_zone;
        return;
    }

    t_zone *curr = *list;
    while (curr->next)
        curr = curr->next;

    curr->next     = new_zone;
    new_zone->prev = curr;
}

/*
 * Removes a zone from a zone list.
 */
void zone_remove(t_zone **list, t_zone *const z)
{
    if (!list || !*list || !z)
        return;

    if (z->prev)
        z->prev->next = z->next;
    else
        *list = z->next;

    if (z->next)
        z->next->prev = z->prev;
}

/*
 * Helper to search a specific zone for a payload pointer.
 * Reduces cognitive complexity.
 */
static bool chunk_search_in_zone(const t_zone *const z, const void *const ptr, t_zone **out_zone,
                                 t_chunk **out_chunk)
{
    const t_chunk *c = OFFSET_PTR(t_chunk, z, ZONE_HEADER_SIZE);

    while ((const void *) c < OFFSET_PTR(const void, z, z->size) && is_valid_chunk(c, z)) {
        if (chunk_to_payload(c) == ptr) {
            if (out_zone)
                *out_zone = (t_zone *) z;
            if (out_chunk)
                *out_chunk = (t_chunk *) c;
            return (true);
        }
        c = chunk_get_next(c, z);
        if (!c)
            break;
    }
    return (false);
}

/*
 * Validates whether a given arbitrary pointer is exactly
 * a payload pointer within the provided zone list.
 * Safely iterates chunks without risking arbitrary segfaults.
 */
bool zone_contains_ptr(const t_zone *const list, const void *const ptr, t_zone **out_zone,
                       t_chunk **out_chunk)
{
    if (!list || !ptr)
        return (false);

    const t_zone *z = list;

    while (z) {
        // Broad boundary check before diving into chunks
        if (ptr > (const void *) z && ptr < OFFSET_PTR(const void, z, z->size)) {
            if (chunk_search_in_zone(z, ptr, out_zone, out_chunk))
                return (true);
        }
        z = z->next;
    }
    return (false);
}
