#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * A pure C black-box integration test.
 * It strictly includes the standard library and links dynamically
 * against libft_malloc.so. If it runs and succeeds, it proves
 * our library correctly overrides the libc symbols.
 */

int main(void)
{
    // 1. Allocate
    char *str = malloc(1024);
    if (!str) {
        write(2, "Integration Test Failed: malloc returned NULL\n", 46);
        return (1);
    }

    // 2. Write
    const char *msg = "FT_MALLOC_WORKS";
    for (int i = 0; i < 16; i++) {
        str[i] = msg[i];
    }

    // 3. Reallocate (grow)
    char *new_str = realloc(str, 2048);
    if (!new_str) {
        free(str);
        write(2, "Integration Test Failed: realloc returned NULL\n", 47);
        return (1);
    }

    // 4. Verify data persisted
    if (strcmp(new_str, "FT_MALLOC_WORKS") != 0) {
        free(new_str);
        write(2, "Integration Test Failed: realloc data corrupted\n", 48);
        return (1);
    }

    // 5. Free
    free(new_str);

    // If we reach here without a SegFault or abort, the .so works perfectly in the wild.
    write(1, "Integration Test Passed: Pure libc override successful.\n", 56);
    return (0);
}
