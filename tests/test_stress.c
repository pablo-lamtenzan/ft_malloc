#include "ft_malloc.h"

#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STRESS_ALLOCS 5000

// Helper to shuffle the array of pointers for randomized free orders
static void shuffle(void **array, size_t n)
{
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
            size_t j = i + (rand() / ((RAND_MAX / (n - i)) + 1));
            void  *t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

static void run_stress_allocations(void **ptrs, size_t *sizes)
{
    for (int i = 0; i < STRESS_ALLOCS; i++) {
        sizes[i] = (rand() % 8000) + 1; // Range: 1 to 8000 bytes
        ptrs[i]  = ft_malloc(sizes[i]);
        cr_assert_not_null(ptrs[i], "Malloc failed at iteration %d with size %zu", i, sizes[i]);

        // Write boundary bytes to ensure contiguous allocations don't stomp each other
        memset(ptrs[i], 0xAA, sizes[i]); // NOLINT
    }
}

static void assert_stress_block(const unsigned char *p, size_t size)
{
    cr_assert(p[0] == 0xAA && p[size - 1] == 0xAA);
}

static void run_stress_validations(void **ptrs, const size_t *sizes)
{
    for (int i = 0; i < STRESS_ALLOCS; i++) {
        assert_stress_block((unsigned char *) ptrs[i], sizes[i]);
    }
}

static void run_stress_frees(void **ptrs)
{
    for (int i = 0; i < STRESS_ALLOCS; i++) {
        ft_free(ptrs[i]);
    }
}

Test(ft_malloc, stress_randomized_allocs)
{
    srand(time(NULL));

    void  *ptrs[STRESS_ALLOCS];
    size_t sizes[STRESS_ALLOCS];

    // 1. Allocate massive random blocks (triggering TINY, SMALL, and LARGE multiplexing)
    run_stress_allocations(ptrs, sizes);

    // 2. Validate data consistency before any frees happen
    run_stress_validations(ptrs, sizes);

    // 3. Shuffle pointers to trigger multi-directional coalescing violently
    shuffle(ptrs, STRESS_ALLOCS);

    // 4. Free completely randomly
    run_stress_frees(ptrs);

    // 5. Ultimate White-Box Assertion
    // If our coalescing logic and unmap thresholds are mathematically flawless,
    // all zones must have naturally collapsed and returned themselves to the OS.
    cr_assert_null(g_arena.tiny_zones, "TINY Zones leaked during stress test!");
    cr_assert_null(g_arena.small_zones, "SMALL Zones leaked during stress test!");
    cr_assert_null(g_arena.large_zones, "LARGE Zones leaked during stress test!");
}
