#include <stdint.h>
#include <stddef.h>

/**
 * @brief Flags to indicate the capabilities of the various memory systems
 */
#define MALLOC_CAP_EXEC ( 1 << 0 )     ///< Memory must be able to run executable code
#define MALLOC_CAP_32BIT ( 1 << 1 )    ///< Memory must allow for aligned 32-bit data accesses
#define MALLOC_CAP_8BIT ( 1 << 2 )     ///< Memory must allow for 8/16/...-bit data accesses
#define MALLOC_CAP_DMA ( 1 << 3 )      ///< Memory must be able to accessed by DMA
#define MALLOC_CAP_PID2 ( 1 << 4 )     ///< Memory must be mapped to PID2 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID3 ( 1 << 5 )     ///< Memory must be mapped to PID3 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID4 ( 1 << 6 )     ///< Memory must be mapped to PID4 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID5 ( 1 << 7 )     ///< Memory must be mapped to PID5 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID6 ( 1 << 8 )     ///< Memory must be mapped to PID6 memory space (PIDs are not currently used)
#define MALLOC_CAP_PID7 ( 1 << 9 )     ///< Memory must be mapped to PID7 memory space (PIDs are not currently used)
#define MALLOC_CAP_SPIRAM ( 1 << 10 )  ///< Memory must be in SPI RAM
#define MALLOC_CAP_INTERNAL \
    ( 1 << 11 )  ///< Memory must be internal; specifically it should not disappear when flash/spiram cache is switched
                 ///< off
#define MALLOC_CAP_DEFAULT \
    ( 1 << 12 )  ///< Memory can be returned in a non-capability-specific memory allocation (e.g. malloc(), calloc())
                 ///< call
#define MALLOC_CAP_INVALID ( 1 << 31 )  ///< Memory can't be used / list end marker

/** @brief Structure to access heap metadata via multi_heap_get_info */
typedef struct {
    size_t total_free_bytes;       ///<  Total free bytes in the heap. Equivalent to multi_free_heap_size().
    size_t total_allocated_bytes;  ///<  Total bytes allocated to data in the heap.
    size_t largest_free_block;     ///<  Size of largest free block in the heap. This is the largest malloc-able size.
    size_t minimum_free_bytes;     ///<  Lifetime minimum free heap size. Equivalent to multi_minimum_free_heap_size().
    size_t allocated_blocks;       ///<  Number of (variable size) blocks allocated in the heap.
    size_t free_blocks;            ///<  Number of (variable size) free blocks in the heap.
    size_t total_blocks;           ///<  Total number of (variable size) blocks in the heap.
} multi_heap_info_t;

void heap_caps_get_info( multi_heap_info_t* info, uint32_t caps );
