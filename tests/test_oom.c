#include "ft_malloc.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>

Test(ft_malloc, oom_graceful_degradation)
{
    struct rlimit old_limit;
    struct rlimit new_limit;

    // 1. Save original limit
    int ret = getrlimit(RLIMIT_AS, &old_limit);
    cr_assert_eq(ret, 0, "getrlimit failed");

    // 2. Choke the process virtual memory tightly to 10MB
    new_limit.rlim_cur = (rlim_t) 10 * 1024 * 1024;
    new_limit.rlim_max = old_limit.rlim_max;

    // Some systems restrict lowering hard limits, so we only clamp the soft limit.
    ret = setrlimit(RLIMIT_AS, &new_limit);
    cr_assert_eq(ret, 0, "setrlimit failed to apply OOM choke");

    // 3. Attempt a massive allocation that instantly blows the 10MB limit
    // We expect sys_mmap to fail internally, returning MAP_FAILED, and ft_malloc to propagate NULL.
    void *massive_ptr = ft_malloc((size_t) 100 * 1024 * 1024);

    // 4. Assert graceful failure
    cr_assert_null(massive_ptr, "ft_malloc should have returned NULL under extreme OOM pressure");

    // Assert arena remained unpolluted by the aborted mapping
    cr_assert_null(g_arena.large_zones, "Large zone list must remain empty after aborted mmap");

    // 5. Restore original limit to allow Criterion to exit safely
    setrlimit(RLIMIT_AS, &old_limit);
}
