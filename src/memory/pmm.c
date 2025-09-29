#include "pmm.h"
#include "multiboot/multiboot.h"
#include "vga.h"
#include "std/string.h"
#include "std/stddef.h"

// Symbols from linker.ld
extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];

// Global PMM state
static page_bitmap_t page_bitmap = {0};
static memory_region_t *memory_regions = NULL;
static uint64_t kernel_end = 0;

// Helper functions
uint64_t align_up(uint64_t addr, uint64_t align) {
    return (addr + align - 1) & ~(align - 1);
}

uint64_t align_down(uint64_t addr, uint64_t align) {
    return addr & ~(align - 1);
}

// Set bit in bitmap (mark page as used)
static void bitmap_set(uint64_t page_index) {
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    page_bitmap.bitmap[byte_index] |= (1 << bit_index);
}

// Clear bit in bitmap (mark page as free)
static void bitmap_clear(uint64_t page_index) {
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    page_bitmap.bitmap[byte_index] &= ~(1 << bit_index);
}

// Test bit in bitmap (check if page is used)
static bool bitmap_test(uint64_t page_index) {
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    return page_bitmap.bitmap[byte_index] & (1 << bit_index);
}

// Find first free page
static uint64_t find_free_page(void) {
    for (uint64_t i = 0; i < page_bitmap.total_pages; i++) {
        if (!bitmap_test(i)) {
            return i;
        }
    }
    return (uint64_t)-1; // No free pages
}

// Add memory region to linked list
static void add_memory_region(uint64_t base, uint64_t size, uint32_t type) {
    // For simplicity, we'll use a static array for now
    // In a real implementation, you'd dynamically allocate this
    static memory_region_t regions[32];
    static int region_count = 0;
    
    if (region_count >= 32) return;
    
    regions[region_count].base = base;
    regions[region_count].size = size;
    regions[region_count].type = type;
    regions[region_count].next = memory_regions;
    memory_regions = &regions[region_count];
    region_count++;
}

// Parse multiboot memory map
static void parse_memory_map(void *mbi) {
    uint8_t *ptr = (uint8_t*)mbi + 8; // multiboot2: skip total_size + reserved
    
    for (;;) {
        struct multiboot_tag *tag = (struct multiboot_tag*)ptr;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct multiboot_tag_mmap *m = (struct multiboot_tag_mmap*)tag;
            uint8_t *entry = ptr + sizeof(*m);
            uint8_t *end = ptr + tag->size;
            
            while (entry < end) {
                struct multiboot_mmap_entry *e = (struct multiboot_mmap_entry*)entry;
                add_memory_region(e->addr, e->len, e->type);
                entry += m->entry_size;
            }
        }
        
        // tags are 8-byte aligned
        ptr += (tag->size + MULTIBOOT_TAG_ALIGN_MASK) & ~((uint32_t)MULTIBOOT_TAG_ALIGN_MASK);
    }
}

void pmm_init(void *multiboot_info) {
    puts("Initializing Physical Memory Manager...\n");

    // Parse memory map from multiboot
    parse_memory_map(multiboot_info);

    // Find the highest usable address to determine total pages
    uint64_t highest_addr = 0;
    uint64_t max_reasonable_memory = 0x20000000ULL; // Limit to 512MB for now

    memory_region_t *region = memory_regions;
    while (region) {
        if (region->type == MEMORY_TYPE_AVAILABLE) {
            uint64_t region_end = region->base + region->size;
            if (region_end > highest_addr && region_end <= max_reasonable_memory) {
                highest_addr = region_end;
            }
        }
        region = region->next;
    }

    if (highest_addr == 0) {
        highest_addr = 0x8000000ULL; // 128MB minimum
    }

    // Calculate total pages
    page_bitmap.total_pages = align_up(highest_addr, PAGE_SIZE) / PAGE_SIZE;

    // Find a place for bitmap (use physical address directly for now)
    kernel_end = (uint64_t)_kernel_end; // Get real kernel end from linker
    uint64_t bitmap_size = (page_bitmap.total_pages + 7) / 8; // Round up to bytes
    uint64_t bitmap_addr = align_up(kernel_end, PAGE_SIZE);

    page_bitmap.bitmap = (uint8_t*)bitmap_addr;

    // Debug output
    puts("Kernel end address: 0x"); puts_hex64(kernel_end); puts("\n");
    puts("Total pages to manage: "); puts_uint64(page_bitmap.total_pages); puts("\n");
    puts("Bitmap size: "); puts_uint64(bitmap_size); puts(" bytes\n");
    puts("Bitmap location: 0x"); puts_hex64(bitmap_addr); puts("\n");

    // Initially mark all pages as used (set all bits to 1)
    memset((void*)bitmap_addr, 0xFF, bitmap_size);
    page_bitmap.used_pages = page_bitmap.total_pages;
    page_bitmap.free_pages = 0;

    // Mark available regions as free
    region = memory_regions;
    uint64_t freed_pages = 0;
    while (region) {
        if (region->type == MEMORY_TYPE_AVAILABLE) {
            uint64_t start_page = align_up(region->base, PAGE_SIZE) / PAGE_SIZE;
            uint64_t end_page = align_down(region->base + region->size, PAGE_SIZE) / PAGE_SIZE;

            if (end_page > page_bitmap.total_pages) {
                end_page = page_bitmap.total_pages;
            }

            if (start_page >= page_bitmap.total_pages) {
                region = region->next;
                continue;
            }

            for (uint64_t page = start_page; page < end_page; page++) {
                if (page < page_bitmap.total_pages && bitmap_test(page)) {
                    bitmap_clear(page);
                    page_bitmap.used_pages--;
                    page_bitmap.free_pages++;
                    freed_pages++;
                }
            }
        }
        region = region->next;
    }

    // Mark kernel and bitmap area as used
    uint64_t kernel_start_page = 0x100000 / PAGE_SIZE; // 1MB
    uint64_t bitmap_end = bitmap_addr + bitmap_size;
    uint64_t used_end_page = align_up(bitmap_end, PAGE_SIZE) / PAGE_SIZE;

    for (uint64_t page = kernel_start_page; page < used_end_page; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            page_bitmap.used_pages++;
            page_bitmap.free_pages--;
        }
    }

    puts("PMM initialized.\n");
    puts("Total pages: "); puts_uint64(page_bitmap.total_pages); puts("\n");
    puts("Free pages: "); puts_uint64(page_bitmap.free_pages); puts("\n");
    puts("Used pages: "); puts_uint64(page_bitmap.used_pages); puts("\n");

    // Test find_free_page
    uint64_t first_free = find_free_page();
    puts("First free page index: "); puts_uint64(first_free); puts("\n");
    if (first_free != (uint64_t)-1) {
        puts("First free page address: 0x"); puts_hex64(first_free * PAGE_SIZE); puts("\n");
    } else {
        puts("ERROR: No free pages found!\n");
    }

    // Test pmm_alloc_page
    uint64_t test_page = pmm_alloc_page();
    if (!test_page) {
        puts("ERROR: PMM alloc failed!\n");
    } else {
        puts("PMM works, first allocated page: 0x"); puts_hex64(test_page); puts("\n");
    }
}

uint64_t pmm_alloc_page(void) {
    // Check if PMM is initialized
    if (page_bitmap.bitmap == NULL || page_bitmap.total_pages == 0) {
        puts("ERROR: PMM not initialized!\n");
        return 0;
    }
    
    uint64_t page_index = find_free_page();
    if (page_index == (uint64_t)-1) {
        puts("ERROR: Out of physical memory!\n");
        puts("Free pages: "); puts_uint64(page_bitmap.free_pages); puts("\n");
        puts("Used pages: "); puts_uint64(page_bitmap.used_pages); puts("\n");
        return 0; // Out of memory
    }
    
    bitmap_set(page_index);
    page_bitmap.used_pages++;
    page_bitmap.free_pages--;
    
    return page_index * PAGE_SIZE;
}

uint64_t pmm_alloc_pages(uint64_t num_pages) {
    uint64_t start_page = (uint64_t)-1;
    uint64_t count = 0;

    for (uint64_t i = 0; i < page_bitmap.total_pages; i++) {
        if (!bitmap_test(i)) {
            if (count == 0) {
                start_page = i;
            }
            count++;

            if (count == num_pages) {
                for (uint64_t j = start_page; j < start_page + num_pages; j++) {
                    bitmap_set(j);
                }
                page_bitmap.used_pages += num_pages;
                page_bitmap.free_pages -= num_pages;
                return start_page * PAGE_SIZE;
            }
        } else {
            count = 0;
        }
    }

    puts("ERROR: Not enough contiguous pages available!\n");
    return 0;
}

void pmm_free_page(uint64_t page_addr) {
    uint64_t page_index = page_addr / PAGE_SIZE;
    
    if (page_index >= page_bitmap.total_pages) {
        return; // Invalid page
    }
    
    if (!bitmap_test(page_index)) {
        return; // Already free
    }
    
    bitmap_clear(page_index);
    page_bitmap.used_pages--;
    page_bitmap.free_pages++;
}

uint64_t pmm_get_free_memory(void) {
    return page_bitmap.free_pages * PAGE_SIZE;
}

uint64_t pmm_get_used_memory(void) {
    return page_bitmap.used_pages * PAGE_SIZE;
}

void pmm_dump_memory_map(void) {
    puts("=== Physical Memory Map ===\n");
    memory_region_t *region = memory_regions;
    while (region) {
        puts("0x"); puts_hex64(region->base);
        puts(" - 0x"); puts_hex64(region->base + region->size - 1);
        puts(" ("); puts_uint64(region->size); puts(" bytes) - ");
        
        switch (region->type) {
            case MEMORY_TYPE_AVAILABLE:
                puts("Available\n");
                break;
            case MEMORY_TYPE_RESERVED:
                puts("Reserved\n");
                break;
            case MEMORY_TYPE_ACPI_RECLAIMABLE:
                puts("ACPI Reclaimable\n");
                break;
            case MEMORY_TYPE_ACPI_NVS:
                puts("ACPI NVS\n");
                break;
            case MEMORY_TYPE_BAD_RAM:
                puts("Bad RAM\n");
                break;
            default:
                puts("Unknown ("); puts_uint64(region->type); puts(")\n");
                break;
        }
        region = region->next;
    }
}
