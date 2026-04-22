#include "ft_malloc.h"

#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdio.h>
#include <string.h>

Test(ft_malloc, show_alloc_mem_format, .init = cr_redirect_stdout)
{
    // Arrange: Perform allocations spanning all three zones
    void *tiny  = ft_malloc(42);
    void *small = ft_malloc(1024);
    void *large = ft_malloc(8192);

    cr_assert_not_null(tiny);
    cr_assert_not_null(small);
    cr_assert_not_null(large);

    // Act: Write the memory map out to intercepted stdout
    show_alloc_mem();

    // We must manually flush because our internal ft_putchar uses write(1, ...)
    // which bypasses libc's stdout buffering. Criterion's redirect natively
    // captures fd 1, so flushing is required to sync the stream reading.
    fflush(stdout);

    // Assert: Extract the captured output string
    FILE *f_out = cr_get_redirected_stdout();
    cr_assert_not_null(f_out, "Failed to capture redirected stdout");

    char   buffer[4096] = {0};
    size_t bytes_read   = fread(buffer, 1, sizeof(buffer) - 1, f_out);

    cr_assert_gt(bytes_read, 0, "show_alloc_mem produced no output");

    // We must mathematically verify exactly what was requested.
    // Alignment may push the actual payload size slightly higher, but
    // the output should correctly display the tracked payload boundaries.

    // 1. Verify headers
    cr_assert_not_null(strstr(buffer, "TINY : 0x"), "Output missing TINY zone header");
    cr_assert_not_null(strstr(buffer, "SMALL : 0x"), "Output missing SMALL zone header");
    cr_assert_not_null(strstr(buffer, "LARGE : 0x"), "Output missing LARGE zone header");

    // 2. Verify total sum formatting
    cr_assert_not_null(strstr(buffer, "Total : 13312 bytes"), "Output missing correct Total sum");

    // 3. Cleanup so we don't pollute subsequent tests
    ft_free(tiny);
    ft_free(small);
    ft_free(large);
}
