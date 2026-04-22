#include "ft_malloc.h"

#include <criterion/criterion.h>

#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wuse-after-free"
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wanalyzer-free-of-non-heap"
#pragma GCC diagnostic ignored "-Wanalyzer-double-free"

Test(ft_malloc, free_invalid_pointers)
{
    // The program should not crash (SegFault) on these
    ft_free(NULL); // NOLINT

    int x = 42;
    ft_free(&x); // NOLINT

    void *ptr = (void *) 0xDEADBEEF;
    ft_free(ptr); // NOLINT

    cr_assert_eq(x, 42); // Stack variable untouched
    cr_assert_null(g_arena.tiny_zones);
    cr_assert_null(g_arena.small_zones);
    cr_assert_null(g_arena.large_zones);
}

Test(ft_malloc, free_coalesce_forward)
{
    void *ptr_a = ft_malloc(16);
    cr_assert_not_null(ptr_a);
    void *ptr_b = ft_malloc(16);
    cr_assert_not_null(ptr_b);
    void *dummy = ft_malloc(16); // Prevent zone unmap

    t_chunk *c_a         = payload_to_chunk(ptr_a);
    size_t   size_a_init = CHUNK_SIZE(c_a);

    ft_free(ptr_b); // B is now free
    ft_free(ptr_a); // A should merge forward into B

    cr_assert_gt(CHUNK_SIZE(c_a), size_a_init, "Chunk A should have absorbed Chunk B");
    ft_free(dummy);
}

Test(ft_malloc, free_coalesce_backward)
{
    void *ptr_a = ft_malloc(16);
    cr_assert_not_null(ptr_a);
    void *ptr_b = ft_malloc(16);
    cr_assert_not_null(ptr_b);
    void *dummy = ft_malloc(16); // Prevent zone unmap

    t_chunk *c_a         = payload_to_chunk(ptr_a);
    size_t   size_a_init = CHUNK_SIZE(c_a);

    ft_free(ptr_a); // A is now free
    ft_free(ptr_b); // B detects A is free via prev_size and merges backward

    cr_assert_eq(IS_FREE(c_a), FLAG_FREE);
    cr_assert_gt(CHUNK_SIZE(c_a), size_a_init, "Chunk B should have merged backward into Chunk A");
    // c_b is now logically erased/absorbed
    ft_free(dummy);
}

Test(ft_malloc, free_coalesce_middle)
{
    void *ptr_a = ft_malloc(16);
    cr_assert_not_null(ptr_a);
    void *ptr_b = ft_malloc(16);
    cr_assert_not_null(ptr_b);
    void *ptr_c = ft_malloc(16);
    cr_assert_not_null(ptr_c);
    void *dummy = ft_malloc(16); // Prevent zone unmap

    t_chunk *c_a            = payload_to_chunk(ptr_a);
    size_t   initial_a_size = CHUNK_SIZE(c_a);

    ft_free(ptr_a);
    ft_free(ptr_c);
    // Currently: [FREE] [ALLOCATED B] [FREE]

    ft_free(ptr_b);
    // B should detect PREV_FREE, merge into A, and then forward merge into C.

    cr_assert_gt(CHUNK_SIZE(c_a), initial_a_size * 2, "A should have swallowed both B and C");
    ft_free(dummy);
}

Test(ft_malloc, free_zone_unmap)
{
    void *ptr = ft_malloc(TINY_MAX);
    cr_assert_not_null(g_arena.tiny_zones, "Zone mapped");

    ft_free(ptr);
    cr_assert_null(g_arena.tiny_zones, "Zone unmapped completely upon monolithic free");
}
