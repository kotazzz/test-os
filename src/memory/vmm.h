#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>
#include "std/stddef.h"

// Page table entry flags
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1) 
#define PAGE_USER       (1 << 2)
#define PAGE_WRITE_THROUGH (1 << 3)
#define PAGE_CACHE_DISABLE (1 << 4)
#define PAGE_ACCESSED   (1 << 5)
#define PAGE_DIRTY      (1 << 6)
#define PAGE_HUGE       (1 << 7)
#define PAGE_GLOBAL     (1 << 8)
#define PAGE_NO_EXECUTE (1ULL << 63)

// Virtual memory regions
#define KERNEL_VIRTUAL_BASE 0xFFFF800000000000ULL  // Higher half kernel
#define USER_VIRTUAL_BASE   0x0000000000400000ULL  // 4MB for user space

// Page table structures for x86_64
typedef struct {
    uint64_t entries[512];
} page_table_t;

// Buddy system parameters
#define MAX_ORDER 10 // Maximum order for buddy system (2^10 = 1024 pages)

typedef struct buddy_block {
    struct buddy_block *next;
} buddy_block_t;

extern buddy_block_t *free_lists[MAX_ORDER + 1];

// VMM functions
void vmm_init(void);
bool vmm_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
void vmm_unmap_page(uint64_t virtual_addr);
uint64_t vmm_get_physical_addr(uint64_t virtual_addr);
bool vmm_is_mapped(uint64_t virtual_addr);

// Helper function for page table traversal
uint64_t *get_page_table_entry(uint64_t virtual_addr, bool create);

// Kernel heap functions (simple bump allocator for now)
void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size, size_t alignment);
void kfree(void *ptr); // Will be implemented later

// Page directory manipulation
void vmm_load_page_directory(uint64_t pml4_phys);
uint64_t vmm_get_current_page_directory(void);

// Debug function to get kernel heap info
uint64_t vmm_get_heap_info(uint64_t *start, uint64_t *current, uint64_t *end);

// Check if VMM is properly initialized
bool vmm_is_initialized(void);

#endif // VMM_H
