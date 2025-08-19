#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stdbool.h>

// Page size is 4KB
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

// Memory types from multiboot
#define MEMORY_TYPE_AVAILABLE 1
#define MEMORY_TYPE_RESERVED 2
#define MEMORY_TYPE_ACPI_RECLAIMABLE 3
#define MEMORY_TYPE_ACPI_NVS 4
#define MEMORY_TYPE_BAD_RAM 5

// Bitmap structure for tracking page allocation
typedef struct {
    uint8_t *bitmap;        // Bitmap array
    uint64_t total_pages;   // Total number of pages
    uint64_t used_pages;    // Number of allocated pages  
    uint64_t free_pages;    // Number of free pages
} page_bitmap_t;

// Memory region structure
typedef struct memory_region {
    uint64_t base;          // Start address
    uint64_t size;          // Size in bytes
    uint32_t type;          // Memory type
    struct memory_region *next;
} memory_region_t;

// PMM functions
void pmm_init(void *multiboot_info);
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t page_addr);
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);
void pmm_dump_memory_map(void);

// Helper functions
uint64_t align_up(uint64_t addr, uint64_t align);
uint64_t align_down(uint64_t addr, uint64_t align);

#endif // PMM_H
