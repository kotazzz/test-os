#include "multiboot.h"
#include "../vga.h"

uint64_t parse_multiboot2_memory_map(void *mbi) {
    uint8_t *ptr = (uint8_t*)mbi + 8; // multiboot2: skip total_size + reserved
    uint64_t total_usable = 0;

    for (;;) {
        struct multiboot_tag *tag = (struct multiboot_tag*)ptr;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct multiboot_tag_mmap *m = (struct multiboot_tag_mmap*)tag;
            uint8_t *entry = ptr + sizeof(*m);
            uint8_t *end = ptr + tag->size;
            while (entry < end) {
                struct multiboot_mmap_entry *e = (struct multiboot_mmap_entry*)entry;
                if (e->type == 1) { // 1 == usable RAM
                    total_usable += e->len;
                }
                entry += m->entry_size;
            }
        }
        // tags are 8-byte aligned
        ptr += (tag->size + MULTIBOOT_TAG_ALIGN_MASK) & ~((uint32_t)MULTIBOOT_TAG_ALIGN_MASK);
    }

    return total_usable; // bytes
}

void parse_multiboot2_tags(void *mbi) {
    uint8_t *ptr = (uint8_t*)mbi + 8; // multiboot2: skip total_size + reserved
    
    puts("=== Multiboot2 Information ===\n");
    
    for (;;) {
        struct multiboot_tag *tag = (struct multiboot_tag*)ptr;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            puts("End of multiboot tags\n\n");
            break;
        }

        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_BOOT_COMMAND_LINE: {
                struct multiboot_tag_string *cmdline = (struct multiboot_tag_string*)tag;
                puts("Boot command line: ");
                puts(cmdline->string);
                puts("\n");
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME: {
                struct multiboot_tag_string *loader = (struct multiboot_tag_string*)tag;
                puts("Boot loader name: ");
                puts(loader->string);
                puts("\n");
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
                struct multiboot_tag_basic_meminfo *meminfo = (struct multiboot_tag_basic_meminfo*)tag;
                puts("Basic memory info:\n");
                puts("  Lower memory: ");
                puts_uint64(meminfo->mem_lower);
                puts(" KB\n");
                puts("  Upper memory: ");
                puts_uint64(meminfo->mem_upper);
                puts(" KB\n");
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_BOOTDEV: {
                struct multiboot_tag_bootdev *bootdev = (struct multiboot_tag_bootdev*)tag;
                puts("Boot device: 0x");
                puts_hex64(bootdev->biosdev);
                puts(", slice: ");
                puts_uint64(bootdev->slice);
                puts(", part: ");
                puts_uint64(bootdev->part);
                puts("\n");
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_MODULE: {
                struct multiboot_tag_module *mod = (struct multiboot_tag_module*)tag;
                puts("Module: 0x");
                puts_hex64(mod->mod_start);
                puts(" - 0x");
                puts_hex64(mod->mod_end);
                puts(" (");
                puts(mod->cmdline);
                puts(")\n");
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_MMAP: {
                struct multiboot_tag_mmap *m = (struct multiboot_tag_mmap*)tag;
                puts("Memory map:\n");
                uint8_t *entry = ptr + sizeof(*m);
                uint8_t *end = ptr + tag->size;
                int count = 0;
                while (entry < end && count < 3) { // Limit output to first 3 entries
                    struct multiboot_mmap_entry *e = (struct multiboot_mmap_entry*)entry;
                    puts("  0x");
                    puts_hex64(e->addr);
                    puts(" - 0x");
                    puts_hex64(e->addr + e->len - 1);
                    puts(" (");
                    puts_uint64(e->len);
                    puts(" bytes) - Type: ");
                    puts_uint64(e->type);
                    if (e->type == 1) puts(" (Available)");
                    else if (e->type == 2) puts(" (Reserved)");
                    else if (e->type == 3) puts(" (ACPI Reclaimable)");
                    else if (e->type == 4) puts(" (ACPI NVS)");
                    else if (e->type == 5) puts(" (Bad RAM)");
                    puts("\n");
                    entry += m->entry_size;
                    count++;
                }
                if (entry < end) {
                    puts("  ... (more entries)\n");
                }
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
                struct multiboot_tag_framebuffer *fb = (struct multiboot_tag_framebuffer*)tag;
                puts("Framebuffer: ");
                puts_uint64(fb->framebuffer_width);
                puts("x");
                puts_uint64(fb->framebuffer_height);
                puts("x");
                puts_uint64(fb->framebuffer_bpp);
                puts(" at 0x");
                puts_hex64(fb->framebuffer_addr);
                puts("\n");
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_VBE:
                puts("VBE info tag present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
                puts("ELF sections info present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_APM:
                puts("APM table present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_EFI32:
                puts("EFI 32-bit system table present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_EFI64:
                puts("EFI 64-bit system table present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_SMBIOS:
                puts("SMBIOS tables present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_ACPI_OLD:
                puts("ACPI (old) RSDP present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_ACPI_NEW:
                puts("ACPI (new) RSDP present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_NETWORK:
                puts("Network info present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_EFI_MMAP:
                puts("EFI memory map present\n");
                break;
                
            case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR: {
                uint32_t *base_addr = (uint32_t*)(ptr + 8);
                puts("Load base address: 0x");
                puts_hex64(*base_addr);
                puts("\n");
                break;
            }
            
            default:
                puts("Unknown tag type: ");
                puts_uint64(tag->type);
                puts(" (size: ");
                puts_uint64(tag->size);
                puts(")\n");
                break;
        }
        
        // tags are 8-byte aligned
        ptr += (tag->size + MULTIBOOT_TAG_ALIGN_MASK) & ~((uint32_t)MULTIBOOT_TAG_ALIGN_MASK);
    }
}

void print_multiboot_info(void *mbi) {
    // Parse all multiboot2 tags
    parse_multiboot2_tags(mbi);

    uint64_t total_memory = parse_multiboot2_memory_map(mbi);
    puts("Total usable memory: ");
    puts_uint64(total_memory);
    puts(" bytes\n");
    // kb and mb
    puts("Total usable memory: ");
    puts_uint64(total_memory / 1024);
    puts(" KB\n");
    puts("Total usable memory: ");
    puts_uint64(total_memory / (1024 * 1024));
    puts(" MB\n");
}
