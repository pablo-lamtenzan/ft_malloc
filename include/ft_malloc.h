#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

/*
 * Alignment
 * Memory returned must be 16-byte aligned.
 * Thus, chunk sizes must be multiples of 16.
 */
#define ALIGNMENT                   16
#define ALIGN(size)                 (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define PAGE_ALIGN(size, page_size) (((size) + ((page_size) - 1)) & ~((page_size) - 1))

/*
 * Pointer arithmetic abstractions
 */
#define OFFSET_PTR(type, base, bytes) ((type *) ((char *) (base) + (bytes)))

/*
 * Chunk Flags (Stored in the lowest bits of chunk->size)
 * Since sizes are multiples of 16, the lowest 4 bits are 0.
 */
#define FLAG_FREE      0x1
#define FLAG_PREV_FREE 0x2

/*
 * Macros to work with chunk size and flags
 */
#define CHUNK_SIZE(c)     ((c)->size & ~(ALIGNMENT - 1))
#define IS_FREE(c)        ((c)->size & FLAG_FREE)
#define IS_PREV_FREE(c)   ((c)->size & FLAG_PREV_FREE)
#define SET_FREE(c)       ((c)->size |= FLAG_FREE)
#define SET_ALLOCATED(c)  ((c)->size &= ~FLAG_FREE)
#define SET_PREV_FREE(c)  ((c)->size |= FLAG_PREV_FREE)
#define SET_PREV_ALLOC(c) ((c)->size &= ~FLAG_PREV_FREE)

/*
 * Chunk Header
 * Implicit free list design:
 * `prev_size` is only valid if the previous chunk is free (FLAG_PREV_FREE).
 */
typedef struct s_chunk
{
    size_t prev_size;
    size_t size;
    // Payload starts here
} t_chunk;

#define CHUNK_HEADER_SIZE sizeof(t_chunk)
#define MIN_CHUNK_SIZE    (ALIGN(CHUNK_HEADER_SIZE + ALIGNMENT))

/*
 * Total chunk footprint for a given payload
 */
#define CHUNK_TOTAL_SIZE(payload_size) (CHUNK_HEADER_SIZE + (payload_size))

/*
 * Zone Header
 * Placed at the very beginning of the mmap'd region.
 */
typedef struct s_zone
{
    size_t         size;
    struct s_zone *next;
    struct s_zone *prev;
} t_zone;

#define ZONE_HEADER_SIZE ALIGN(sizeof(t_zone))

/*
 * Allocation Thresholds
 * TINY  : <= 256 bytes
 * SMALL : <= 4096 bytes
 * LARGE : > 4096 bytes
 */
#define TINY_MAX  256
#define SMALL_MAX 4096

/*
 * Minimum Zone capacities
 * The subject requires at least 100 allocations per zone.
 */
#define MIN_ALLOCS_PER_ZONE 100

/*
 * Global State (Arena)
 */
typedef struct s_malloc_state
{
    pthread_mutex_t lock;
    t_zone         *tiny_zones;
    t_zone         *small_zones;
    t_zone         *large_zones;
} t_malloc_state;

/*
 * Global arena extern declaration
 */
extern t_malloc_state g_arena;

/*
 * Public API Prototypes
 */
void  ft_free(void *ptr);
void *ft_malloc(size_t size);
void *ft_realloc(void *ptr, size_t size);
void  show_alloc_mem(void);

/*
 * libc prototypes for public shared object export
 */
void  free(void *ptr);                 // NOLINT(readability-redundant-declaration)
void *malloc(size_t size);             // NOLINT(readability-redundant-declaration)
void *realloc(void *ptr, size_t size); // NOLINT(readability-redundant-declaration)

/*
 * Internal API Prototypes
 */

// Arena / Init
void arena_lock(void);
void arena_unlock(void);

// OS System Allocator Wrappers
void  *sys_mmap(size_t size);
int    sys_munmap(void *ptr, size_t size);
size_t get_page_size(void);

// Zone operations
t_zone *zone_create(size_t alloc_size);
void    zone_append(t_zone **list, t_zone *new_zone);
void    zone_remove(t_zone **list, t_zone *z);
bool    zone_contains_ptr(const t_zone *zone_list, const void *ptr, t_zone **out_zone,
                          t_chunk **out_chunk);
size_t  calculate_zone_size(size_t max_alloc_size);

// Chunk operations
bool     is_valid_chunk(const t_chunk *c, const t_zone *z);
bool     can_split_chunk(size_t total_size, size_t needed_size);
t_chunk *chunk_split(t_zone *zone, t_chunk *chunk, size_t needed_size);
t_chunk *chunk_coalesce(t_zone *zone, t_chunk *chunk);
t_chunk *chunk_find_free(const t_zone *zone, size_t needed_size);
void    *chunk_to_payload(const t_chunk *chunk);
t_chunk *payload_to_chunk(const void *ptr);
t_chunk *chunk_get_next(const t_chunk *chunk, const t_zone *zone);
t_chunk *chunk_get_prev(const t_chunk *chunk);
