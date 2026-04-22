#include "ft_malloc.h"

#include <criterion/criterion.h>
#include <string.h>

Test(ft_malloc, realloc_null_ptr)
{
    void *ptr = ft_realloc(NULL, 100);
    cr_assert_not_null(ptr, "Should act like malloc(100)");

    t_chunk *c = payload_to_chunk(ptr);
    cr_assert_eq(IS_FREE(c), 0);

    ft_free(ptr);
}

Test(ft_malloc, realloc_zero_size)
{
    void *ptr     = ft_malloc(100);
    void *new_ptr = ft_realloc(ptr, 0);

    cr_assert_null(new_ptr, "realloc(ptr, 0) should return NULL");
    cr_assert_null(g_arena.tiny_zones, "The zone should be unmapped by the inner free");
}

Test(ft_malloc, realloc_shrink_in_place)
{
    void *ptr = ft_malloc(1024); // Small zone
    cr_assert_not_null(ptr);

    t_chunk *c         = payload_to_chunk(ptr);
    size_t   init_size = CHUNK_SIZE(c);

    void *new_ptr = ft_realloc(ptr, 16); // Shrink to minimum
    cr_assert_eq(new_ptr, ptr, "Should shrink in place, returning same pointer");

    cr_assert_lt(CHUNK_SIZE(c), init_size, "Chunk size should have decreased");

    // Check that a new free chunk was split off correctly
    t_chunk *next = chunk_get_next(c, g_arena.small_zones);
    cr_assert_not_null(next);
    cr_assert_eq(IS_FREE(next), FLAG_FREE, "Trailing split chunk should be free");

    ft_free(new_ptr);
}

Test(ft_malloc, realloc_grow_in_place)
{
    void *ptr_a = ft_malloc(16);
    cr_assert_not_null(ptr_a);

    void *ptr_b = ft_malloc(1024);
    cr_assert_not_null(ptr_b);

    t_chunk *c_a       = payload_to_chunk(ptr_a);
    size_t   init_size = CHUNK_SIZE(c_a);

    ft_free(ptr_b); // B is now free, giving A room to grow

    void *new_ptr = ft_realloc(ptr_a, 500);

    cr_assert_eq(new_ptr, ptr_a, "Should have expanded into B in-place");
    cr_assert_gt(CHUNK_SIZE(c_a), init_size, "Chunk A should have absorbed B");

    ft_free(new_ptr);
}

static void write_hello_world(void *ptr)
{
    const char *msg = "Hello World!";
    for (int i = 0; i < 13; i++) {
        ((char *) ptr)[i] = msg[i];
    }
}

Test(ft_malloc, realloc_grow_relocate)
{
    void *ptr_a = ft_malloc(16);
    void *ptr_b = ft_malloc(16); // Block A's forward growth

    write_hello_world(ptr_a);

    void *new_ptr = ft_realloc(ptr_a, 1024);
    cr_assert_neq(new_ptr, ptr_a, "Should have allocated a completely new chunk");
    cr_assert_str_eq(new_ptr, "Hello World!", "Data must be copied precisely");

    ft_free(new_ptr);
    ft_free(ptr_b);
}
