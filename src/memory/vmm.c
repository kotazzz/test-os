#include "vmm.h"
#include "pmm.h"
#include "vga.h"
#include "std/string.h"
#include "std/stddef.h"

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

// Simple bump allocator for kernel heap
void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Check if heap is initialized
    if (kernel_heap_start == 0 || kernel_heap_end == 0) {
        puts("ERROR: Kernel heap not initialized!\n");
        return NULL;
    }
    
    // Align to 8 bytes
    size = align_up(size, 8);
    
    if (kernel_heap_current + size > kernel_heap_end) {
        puts("Kernel heap exhausted! Requested: "); puts_uint64(size);
        puts(" bytes, Available: "); puts_uint64(kernel_heap_end - kernel_heap_current);
        puts(" bytes\n");
        puts("Current heap pos: 0x"); puts_hex64(kernel_heap_current); puts("\n");
        puts("Heap end: 0x"); puts_hex64(kernel_heap_end); puts("\n");
        return NULL;
    }
    
    void *ptr = (void*)kernel_heap_current;
    kernel_heap_current += size;
    
    return ptr;
}

void *kmalloc_aligned(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0) return NULL;
    
    // Align current position to required alignment
    uint64_t aligned_pos = align_up(kernel_heap_current, alignment);
    
    if (aligned_pos + size > kernel_heap_end) {
        puts("Kernel heap exhausted (aligned allocation)! Requested: "); puts_uint64(size);
        puts(" bytes with alignment "); puts_uint64(alignment);
        puts(", Available: "); puts_uint64(kernel_heap_end - aligned_pos);
        puts(" bytes\n");
        return NULL;
    }
    
    kernel_heap_current = aligned_pos + size;
    return (void*)aligned_pos;
}

void kfree(void *ptr) {
    // Bump allocator doesn't support freeing individual allocations
    // This would need a proper allocator like a free list or buddy system
    (void)ptr; // Suppress unused parameter warning
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
