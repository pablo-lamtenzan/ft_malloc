#include "ft_malloc.h"

#include <criterion/criterion.h>

Test(ft_malloc, malloc_zero_bytes)
{
    void *ptr = ft_malloc(0);
    cr_assert_null(ptr, "malloc(0) should return NULL");
    cr_assert_null(g_arena.tiny_zones, "g_arena should be empty");
    cr_assert_null(g_arena.small_zones, "g_arena should be empty");
    cr_assert_null(g_arena.large_zones, "g_arena should be empty");
}

Test(ft_malloc, malloc_alignment)
{
    void *ptr1 = ft_malloc(3);
    void *ptr2 = ft_malloc(17);

    cr_assert_not_null(ptr1);
    cr_assert_not_null(ptr2);

    cr_assert_eq((size_t) ptr1 % ALIGNMENT, 0, "ptr1 not aligned");
    cr_assert_eq((size_t) ptr2 % ALIGNMENT, 0, "ptr2 not aligned");

    ft_free(ptr1);
    ft_free(ptr2);
}

Test(ft_malloc, malloc_tiny_boundary)
{
    void *ptr = ft_malloc(TINY_MAX);
    cr_assert_not_null(ptr);
    cr_assert_not_null(g_arena.tiny_zones, "Should be allocated in TINY zone");
    cr_assert_null(g_arena.small_zones, "SMALL zone should be empty");

    t_chunk *c = payload_to_chunk(ptr);
    cr_assert_eq(IS_FREE(c), 0, "Chunk should be marked allocated");

    ft_free(ptr);
}

Test(ft_malloc, malloc_small_boundary)
{
    void *ptr = ft_malloc(TINY_MAX + 1);
    cr_assert_not_null(ptr);
    cr_assert_not_null(g_arena.small_zones, "Should be allocated in SMALL zone");
    cr_assert_null(g_arena.tiny_zones, "TINY zone should be empty");

    ft_free(ptr);
}

Test(ft_malloc, malloc_large_boundary)
{
    void *ptr = ft_malloc(SMALL_MAX + 1);
    cr_assert_not_null(ptr);
    cr_assert_not_null(g_arena.large_zones, "Should be allocated in LARGE zone");
    cr_assert_null(g_arena.small_zones, "SMALL zone should be empty");

    t_zone  *z = g_arena.large_zones;
    t_chunk *c = payload_to_chunk(ptr);

    // Large zones are exact fits (+ headers + page alignment)
    cr_assert_eq(z->size, PAGE_ALIGN(ZONE_HEADER_SIZE + ALIGN(SMALL_MAX + 1 + CHUNK_HEADER_SIZE),
                                     get_page_size()));
    cr_assert_eq(CHUNK_SIZE(c), z->size - ZONE_HEADER_SIZE);

    ft_free(ptr);
}

static void allocate_and_assert(void **ptrs, int count)
{
    for (int i = 0; i < count; i++) {
        ptrs[i] = ft_malloc(TINY_MAX);
        cr_assert_not_null(ptrs[i]);
    }
}

static void free_all(void **ptrs, int count)
{
    for (int i = 0; i < count; i++) {
        ft_free(ptrs[i]);
    }
}

Test(ft_malloc, malloc_zone_exhaustion_part1)
{
    void *ptrs[MIN_ALLOCS_PER_ZONE + 10];

    // Exhaust the first TINY zone and force a second one to spawn
    allocate_and_assert(ptrs, MIN_ALLOCS_PER_ZONE + 10);

    cr_assert_not_null(g_arena.tiny_zones);
    cr_assert_not_null(g_arena.tiny_zones->next, "A second TINY zone should have spawned");

    free_all(ptrs, MIN_ALLOCS_PER_ZONE + 10);
}
