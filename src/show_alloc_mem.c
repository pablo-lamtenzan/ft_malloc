#include "ft_malloc.h"

/*
 * Low-level write utilities to avoid dependencies on external libs
 * and especially printf, which can trigger an internal malloc.
 */
static void ft_putchar(char c)
{
    ssize_t ret = write(1, &c, 1);
    (void) ret;
}

static void ft_putstr(const char *s)
{
    while (*s)
        ft_putchar(*s++);
}

static void ft_puthex(unsigned long n)
{
    char *hex = "0123456789ABCDEF";
    if (n >= 16)
        ft_puthex(n / 16);
    ft_putchar(hex[n % 16]);
}

static void ft_putptr(const void *const ptr)
{
    ft_putstr("0x");
    ft_puthex((unsigned long) ptr);
}

static void ft_putnbr(const size_t n)
{
    if (n >= 10)
        ft_putnbr(n / 10);
    ft_putchar((n % 10) + '0');
}

/*
 * Traverses a zone linked list and linearly reads each chunk.
 * Prints only the active chunks in the format specified by the subject.
 */
static size_t print_zone_list(const t_zone *const list, const char *const name)
{
    if (!list || !name)
        return (0);

    size_t        total = 0;
    const t_zone *z     = list;

    while (z) {
        ft_putstr(name);
        ft_putstr(" : ");
        ft_putptr(z);
        ft_putstr("\n");

        const t_chunk *c = OFFSET_PTR(const t_chunk, z, ZONE_HEADER_SIZE);
        while ((const void *) c < OFFSET_PTR(const void, z, z->size) && is_valid_chunk(c, z)) {
            if (!IS_FREE(c)) {
                const void *const start        = chunk_to_payload(c);
                const size_t      payload_size = CHUNK_SIZE(c) - CHUNK_HEADER_SIZE;
                const void *const end          = OFFSET_PTR(const void, start, payload_size);

                ft_putptr(start);
                ft_putstr(" - ");
                ft_putptr(end);
                ft_putstr(" : ");
                ft_putnbr(payload_size);
                ft_putstr(" bytes\n");

                total += payload_size;
            }
            c = chunk_get_next(c, z);
            if (!c)
                break;
        }
        z = z->next;
    }
    return (total);
}

/*
 * Public API.
 * Locks the arena and prints the complete memory map layout.
 */
void show_alloc_mem(void)
{
    arena_lock();

    size_t total = 0;

    total += print_zone_list(g_arena.tiny_zones, "TINY");
    total += print_zone_list(g_arena.small_zones, "SMALL");
    total += print_zone_list(g_arena.large_zones, "LARGE");

    ft_putstr("Total : ");
    ft_putnbr(total);
    ft_putstr(" bytes\n");

    arena_unlock();
}
