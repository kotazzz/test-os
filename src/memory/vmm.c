#include "vmm.h"
#include "pmm.h"
#include "vga.h"
#include "std/string.h"
#include "std/stddef.h"

#define MAX_ORDER 10 // Maximum order for buddy system (2^10 = 1024 pages)

// Current page directory
static uint64_t current_pml4 = 0;

// Simple kernel heap (bump allocator)
static uint64_t kernel_heap_start = 0;
static uint64_t kernel_heap_current = 0;
static uint64_t kernel_heap_end = 0;

// Convert physical address to virtual address (identity mapping assumed)
static inline void* phys_to_virt(uint64_t phys_addr) {
    return (void*)phys_addr;
}

// Get page table entry
uint64_t* get_page_table_entry(uint64_t virtual_addr, bool create) {
    // Extract page table indices from virtual address
    uint64_t pml4_index = (virtual_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_addr >> 12) & 0x1FF;
    
    // Get PML4 (we assume it's identity mapped for now)
    page_table_t *pml4 = (page_table_t*)phys_to_virt(current_pml4);
    
    // Get or create PDP
    if (!(pml4->entries[pml4_index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t pdp_phys = pmm_alloc_page();
        if (!pdp_phys) return NULL;
        
        // Clear the new page table
        memset(phys_to_virt(pdp_phys), 0, PAGE_SIZE);
        pml4->entries[pml4_index] = pdp_phys | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    page_table_t *pdp = (page_table_t*)phys_to_virt(pml4->entries[pml4_index] & ~0xFFFULL);
    
    // Get or create PD
    if (!(pdp->entries[pdp_index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t pd_phys = pmm_alloc_page();
        if (!pd_phys) return NULL;
        
        // Clear the new page table
        memset(phys_to_virt(pd_phys), 0, PAGE_SIZE);
        pdp->entries[pdp_index] = pd_phys | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    page_table_t *pd = (page_table_t*)phys_to_virt(pdp->entries[pdp_index] & ~0xFFFULL);
    
    // Get or create PT
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t pt_phys = pmm_alloc_page();
        if (!pt_phys) return NULL;
        
        // Clear the new page table
        memset(phys_to_virt(pt_phys), 0, PAGE_SIZE);
        pd->entries[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    page_table_t *pt = (page_table_t*)phys_to_virt(pd->entries[pd_index] & ~0xFFFULL);
    
    return &pt->entries[pt_index];
}

void vmm_init(void) {
    puts("Initializing Virtual Memory Manager...\n");

    // Allocate and clear PML4 before identity mapping
    current_pml4 = pmm_alloc_page();
    puts("Allocated PML4 at: 0x"); puts_hex64(current_pml4); puts("\n");
    if (!current_pml4) {
        puts("Failed to allocate PML4.\n");
        return;
    }
    memset(phys_to_virt(current_pml4), 0, PAGE_SIZE);

    // Identity map first 128MB for kernel access
    puts("Identity mapping first 128MB...\n");
    uint64_t identity_map_size = 0x8000000ULL; // 128MB
    for (uint64_t addr = 0; addr < identity_map_size; addr += PAGE_SIZE) {
        if (!vmm_map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE)) {
            puts("Failed to map page for identity mapping.\n");
            return;
        }
    }
    puts("Identity mapping completed\n");

    // Setup kernel heap (starts at 16MB, size 16MB)
    kernel_heap_start = 0x1000000;  // 16MB
    kernel_heap_current = kernel_heap_start;
    kernel_heap_end = kernel_heap_start + 0x1000000; // 16MB of heap space

    puts("Setting up kernel heap...\n");
    puts("Heap start: 0x"); puts_hex64(kernel_heap_start); puts("\n");
    puts("Heap end: 0x"); puts_hex64(kernel_heap_end); puts("\n");
    puts("Heap size: "); puts_uint64((kernel_heap_end - kernel_heap_start) / (1024*1024)); puts(" MB\n");

    // Map heap area
    uint64_t mapped_pages = 0;
    puts("Mapping heap pages...\n");
    for (uint64_t addr = kernel_heap_start; addr < kernel_heap_end; addr += PAGE_SIZE) {
        uint64_t phys = pmm_alloc_page();
        if (!vmm_map_page(addr, phys, PAGE_PRESENT | PAGE_WRITABLE)) {
            puts("Failed to map heap page.\n");
            return;
        }
        mapped_pages++;
    }
    puts("Mapped "); puts_uint64(mapped_pages); puts(" pages for kernel heap\n");

    // Final heap verification
    puts("Final heap verification:\n");
    puts("  kernel_heap_start = 0x"); puts_hex64(kernel_heap_start); puts("\n");
    puts("  kernel_heap_current = 0x"); puts_hex64(kernel_heap_current); puts("\n");
    puts("  kernel_heap_end = 0x"); puts_hex64(kernel_heap_end); puts("\n");
    puts("  Available space = "); puts_uint64(kernel_heap_end - kernel_heap_current); puts(" bytes\n");

    // Load new page directory
    vmm_load_page_directory(current_pml4);

    puts("VMM initialized with identity mapping and kernel heap.\n");
}

bool vmm_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    // Align addresses to page boundaries
    virtual_addr = align_down(virtual_addr, PAGE_SIZE);
    physical_addr = align_down(physical_addr, PAGE_SIZE);
    
    uint64_t *pte = get_page_table_entry(virtual_addr, true);
    if (!pte) {
        return false;
    }
    
    *pte = physical_addr | flags;
    
    // Invalidate TLB entry
    __asm__ volatile ("invlpg (%0)" :: "r" (virtual_addr) : "memory");
    
    return true;
}

void vmm_unmap_page(uint64_t virtual_addr) {
    virtual_addr = align_down(virtual_addr, PAGE_SIZE);
    
    uint64_t *pte = get_page_table_entry(virtual_addr, false);
    if (pte && (*pte & PAGE_PRESENT)) {
        *pte = 0;
        
        // Invalidate TLB entry
        __asm__ volatile ("invlpg (%0)" :: "r" (virtual_addr) : "memory");
    }
}

uint64_t vmm_get_physical_addr(uint64_t virtual_addr) {
    uint64_t *pte = get_page_table_entry(virtual_addr, false);
    if (!pte || !(*pte & PAGE_PRESENT)) {
        return 0;
    }
    
    uint64_t page_offset = virtual_addr & 0xFFF;
    uint64_t physical_page = *pte & ~0xFFFULL;
    
    return physical_page + page_offset;
}

bool vmm_is_mapped(uint64_t virtual_addr) {
    uint64_t *pte = get_page_table_entry(virtual_addr, false);
    return pte && (*pte & PAGE_PRESENT);
}

void vmm_load_page_directory(uint64_t pml4_phys) {
    current_pml4 = pml4_phys;
    __asm__ volatile ("mov %0, %%cr3" :: "r" (pml4_phys) : "memory");
}

uint64_t vmm_get_current_page_directory(void) {
    return current_pml4;
}

// Get current CR3 value from processor
static uint64_t get_current_cr3(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    return cr3;
}

// Initialize buddy allocator
void buddy_init(void) {
    memset(free_lists, 0, sizeof(free_lists));

    // Try to allocate the largest block, fallback to smaller blocks if needed
    uint64_t total_pages = 1 << MAX_ORDER;
    uint64_t base_addr = pmm_alloc_pages(total_pages);
    if (base_addr) {
        // Add the block to the largest free list
        free_lists[MAX_ORDER] = (buddy_block_t *)phys_to_virt(base_addr);
        free_lists[MAX_ORDER]->next = NULL;
    } else {
        puts("WARNING: Failed to allocate large contiguous block, falling back to smaller blocks\n");
        // Try to allocate as many smaller blocks as possible
        for (int order = MAX_ORDER - 1; order >= 0; order--) {
            uint64_t block_size = (1ULL << order);
            int blocks_added = 0;
            while (1) {
                uint64_t addr = pmm_alloc_pages(block_size);
                if (!addr) break;
                buddy_block_t *block = (buddy_block_t *)phys_to_virt(addr);
                block->next = free_lists[order];
                free_lists[order] = block;
                blocks_added++;
            }
            if (blocks_added > 0) {
                puts("Buddy order "); puts_uint64(order); puts(": "); puts_uint64(blocks_added); puts(" blocks added\n");
            }
        }
    }
}

// Allocate memory using buddy system
void *buddy_alloc(size_t size) {
    if (size == 0) return NULL;

    // Calculate the required order
    size = align_up(size, PAGE_SIZE);
    int order = 0;
    while ((1ULL << order) * PAGE_SIZE < size) {
        order++;
    }

    if (order > MAX_ORDER) {
        puts("ERROR: Requested size too large for buddy allocator\n");
        return NULL;
    }

    // Find a free block of the required order or higher
    int current_order = order;
    while (current_order <= MAX_ORDER && !free_lists[current_order]) {
        current_order++;
    }

    if (current_order > MAX_ORDER) {
        puts("ERROR: Out of memory in buddy allocator\n");
        puts("Buddy Allocator State:\n");
        for (int i = 0; i <= MAX_ORDER; i++) {
            puts("Order "); puts_uint64(i); puts(": ");
            buddy_block_t *block = free_lists[i];
            int count = 0;
            while (block) {
                count++;
                block = block->next;
            }
            puts_uint64(count); puts(" blocks\n");
        }
        return NULL;
    }

    // Split blocks until the desired order is reached
    while (current_order > order) {
        buddy_block_t *block = free_lists[current_order];
        free_lists[current_order] = block->next;

        current_order--;
        uint64_t block_addr = (uint64_t)block;
        uint64_t buddy_addr = block_addr + (1ULL << current_order) * PAGE_SIZE;

        free_lists[current_order] = (buddy_block_t *)phys_to_virt(block_addr);
        free_lists[current_order]->next = (buddy_block_t *)phys_to_virt(buddy_addr);
        free_lists[current_order]->next->next = NULL;
    }

    // Allocate the block
    buddy_block_t *allocated_block = free_lists[order];
    free_lists[order] = allocated_block->next;
    return (void *)allocated_block;
}

// Free memory using buddy system
void buddy_free(void *ptr, size_t size) {
    if (!ptr || size == 0) return;

    // Calculate the order of the block
    size = align_up(size, PAGE_SIZE);
    int order = 0;
    while ((1ULL << order) * PAGE_SIZE < size) {
        order++;
    }

    if (order > MAX_ORDER) {
        puts("ERROR: Invalid size for buddy allocator\n");
        return;
    }

    // Add the block back to the free list
    uint64_t block_addr = (uint64_t)ptr;
    buddy_block_t *block = (buddy_block_t *)block_addr;

    // Coalesce with buddy blocks if possible
    while (order < MAX_ORDER) {
        uint64_t buddy_addr = block_addr ^ ((1ULL << order) * PAGE_SIZE);
        buddy_block_t *buddy = (buddy_block_t *)buddy_addr;

        // Check if the buddy is free and in the same order
        buddy_block_t *prev = NULL;
        buddy_block_t *current = free_lists[order];
        while (current) {
            if (current == buddy) {
                // Remove buddy from free list
                if (prev) {
                    prev->next = current->next;
                } else {
                    free_lists[order] = current->next;
                }

                // Merge blocks
                if (block_addr > buddy_addr) {
                    block_addr = buddy_addr;
                }
                block = (buddy_block_t *)block_addr;
                order++;
                break;
            }
            prev = current;
            current = current->next;
        }

        if (!current) {
            break; // Buddy not found, stop coalescing
        }
    }

    // Add the block to the free list
    block->next = free_lists[order];
    free_lists[order] = block;
}

// Debug function to get kernel heap info
uint64_t vmm_get_heap_info(uint64_t *start, uint64_t *current, uint64_t *end) {
    if (start) *start = kernel_heap_start;
    if (current) *current = kernel_heap_current;
    if (end) *end = kernel_heap_end;
    return kernel_heap_end - kernel_heap_current; // Available space
}

// Check if VMM is properly initialized
bool vmm_is_initialized(void) {
    return current_pml4 != 0 && kernel_heap_start != 0 && kernel_heap_end > kernel_heap_start;
}

// Header for kmalloc allocations to track size
typedef struct kmalloc_header {
    size_t size;
    uint64_t magic; // For corruption detection
} kmalloc_header_t;

#define KMALLOC_MAGIC 0xDEADBEEFCAFEBABEULL

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Allocate space for header + requested size
    size_t total_size = sizeof(kmalloc_header_t) + size;
    void *raw_ptr = buddy_alloc(total_size);
    
    if (!raw_ptr) return NULL;
    
    // Set up header
    kmalloc_header_t *header = (kmalloc_header_t *)raw_ptr;
    header->size = total_size;
    header->magic = KMALLOC_MAGIC;
    
    // Return pointer after header
    return (void *)((uint8_t *)raw_ptr + sizeof(kmalloc_header_t));
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    // Get header from pointer
    kmalloc_header_t *header = (kmalloc_header_t *)((uint8_t *)ptr - sizeof(kmalloc_header_t));
    
    // Verify magic number for corruption detection
    if (header->magic != KMALLOC_MAGIC) {
        puts("ERROR: kfree - corrupted block or invalid pointer\n");
        return;
    }
    
    // Clear magic to detect double-free
    header->magic = 0;
    
    // Free the block
    buddy_free((void *)header, header->size);
}

typedef struct aligned_alloc_header {
    void *original_ptr;
    size_t original_size;
} aligned_alloc_header_t;

void *kmalloc_aligned(size_t size, size_t alignment) {
    // Buddy allocator inherently aligns to power-of-two sizes, so we can use it directly
    if (alignment <= PAGE_SIZE) {
        return buddy_alloc(size);
    }

    // For larger alignments, allocate extra space for alignment and header
    size_t total_size = size + alignment + sizeof(aligned_alloc_header_t);
    void *raw_ptr = buddy_alloc(total_size);
    if (!raw_ptr) {
        return NULL;
    }

    uintptr_t raw_addr = (uintptr_t)raw_ptr + sizeof(aligned_alloc_header_t);
    uintptr_t aligned_addr = (raw_addr + alignment - 1) & ~(alignment - 1);

    aligned_alloc_header_t *header = (aligned_alloc_header_t *)(aligned_addr - sizeof(aligned_alloc_header_t));
    header->original_ptr = raw_ptr;
    header->original_size = total_size;

    return (void *)aligned_addr;
}

// Free function for aligned allocations
void kfree_aligned(void *ptr) {
    if (!ptr) return;
    aligned_alloc_header_t *header = (aligned_alloc_header_t *)((uintptr_t)ptr - sizeof(aligned_alloc_header_t));
    buddy_free(header->original_ptr, header->original_size);
}

// Create a new address space (PML4) for a process
uint64_t *vmm_create_address_space(void) {
    // Allocate a page for the new PML4
    uint64_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0) {
        return NULL;
    }

    // Clear the PML4 table
    page_table_t *pml4 = (page_table_t *)pml4_phys;
    memset(pml4, 0, sizeof(page_table_t));

    // Copy kernel mappings (upper half of address space)
    if (!vmm_copy_kernel_mappings((uint64_t *)pml4_phys)) {
        pmm_free_page(pml4_phys);
        return NULL;
    }

    return (uint64_t *)pml4_phys;
}

// Copy kernel mappings to a new address space
bool vmm_copy_kernel_mappings(uint64_t *dest_pml4_phys) {
    page_table_t *dest_pml4 = (page_table_t *)dest_pml4_phys;
    page_table_t *src_pml4 = (page_table_t *)current_pml4;

    // Copy upper half entries (kernel space) - entries 256-511
    for (int i = 256; i < 512; i++) {
        dest_pml4->entries[i] = src_pml4->entries[i];
    }

    return true;
}

// Map a page in user space with user permissions
bool vmm_map_user_page(uint64_t *pml4_phys, uint64_t virtual_addr, uint64_t physical_addr) {
    // Temporarily switch to the target address space
    uint64_t old_cr3 = vmm_get_current_page_directory();
    vmm_load_page_directory((uint64_t)pml4_phys);

    // Map the page with user permissions
    bool result = vmm_map_page(virtual_addr, physical_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    // Switch back to original address space
    vmm_load_page_directory(old_cr3);

    return result;
}

// Destroy an address space and free all associated pages
void vmm_destroy_address_space(uint64_t *pml4_phys) {
    if (!pml4_phys) return;

    page_table_t *pml4 = (page_table_t *)pml4_phys;
    
    // Free user space page tables (entries 0-255)
    // Note: This is a simplified implementation - a full implementation would
    // need to recursively free all page tables and user pages
    for (int i = 0; i < 256; i++) {
        if (pml4->entries[i] & PAGE_PRESENT) {
            // For now, just clear the entries
            // A full implementation should recursively free PDPT, PD, and PT tables
            pml4->entries[i] = 0;
        }
    }

    // Free the PML4 itself
    pmm_free_page((uint64_t)pml4_phys);
}

buddy_block_t *free_lists[MAX_ORDER + 1] = {0};
