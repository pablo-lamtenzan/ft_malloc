#include "ft_malloc.h"

/*
 * Request memory pages from the OS.
 * The memory is zeroed out by mmap (MAP_ANONYMOUS).
 */
void *sys_mmap(const size_t size)
{
    if (size == 0)
        return (NULL);

    void *const ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED)
        return (NULL);
    return (ptr);
}

/*
 * Return memory pages to the OS.
 */
int sys_munmap(void *const ptr, const size_t size)
{
    if (!ptr || size == 0)
        return (-1);
    return (munmap(ptr, size));
}

/*
 * Obtain the system page size dynamically.
 * Subject strictly requires `getpagesize()` on macOS and `sysconf` on Linux.
 */
size_t get_page_size(void)
{
#ifdef __APPLE__
    return ((size_t) getpagesize());
#else
    return ((size_t) sysconf(_SC_PAGESIZE));
#endif
}
