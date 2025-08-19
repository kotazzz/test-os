#include <stdint.h>
#include "vga.h"
#include "interrupts/idt.h"
#include "interrupts/irq.h"
#include "keyboard/input.h"
#include "handlers/timer.h"

#define NULL ((void*)0)


#define IDT_ENTRIES 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_BOOT_COMMAND_LINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_APM 10
#define MULTIBOOT_TAG_TYPE_EFI32 11
#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_SMBIOS 13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_NETWORK 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP 17
#define MULTIBOOT_TAG_TYPE_EFI_BS 18
#define MULTIBOOT_TAG_TYPE_EFI32_IH 19
#define MULTIBOOT_TAG_TYPE_EFI64_IH 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed));

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

struct multiboot_tag_string {
    uint32_t type;
    uint32_t size;
    char string[0];
} __attribute__((packed));

struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed));

struct multiboot_tag_bootdev {
    uint32_t type;
    uint32_t size;
    uint32_t biosdev;
    uint32_t slice;
    uint32_t part;
} __attribute__((packed));

struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[0];
} __attribute__((packed));

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} __attribute__((packed));

#define MULTIBOOT_TAG_ALIGN_MASK 7

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


void kmain(void *mbi){
    row = 0;
    col = 0;
    
    // Clear screen
    for(int i = 0; i < 80 * 25; i++) 
        VGA[i] = (' ' | (0x07 << 8));
    
    puts("TOS - Tiny Operating System\n");
    puts("64-bit kernel with keyboard support\n");
    // puts("Type anything to see echo...\n\n");
    
    // Parse all multiboot2 tags
    parse_multiboot2_tags(mbi);

    uint64_t total_memory =  parse_multiboot2_memory_map(mbi);
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

    // Initialize interrupts (только один раз!)
    init_idt();
    init_irq();
    
    // Enable interrupts
    __asm__ volatile ("sti");

    puts("Interrupts enabled. System ready.\n");
    puts("Type something and press Enter:\n");
    // Mini shell
    char input_buffer[256];
    
    
    puts("Mini Shell - Available commands:\n");
    puts("  ticks  - Show system ticks\n");
    puts("  memory - Show memory information\n");
    puts("  help   - Show this help\n");
    puts("shell> ");
    
    while(1) {
        char* result = gets(input_buffer);
        if (result != NULL) {
            // Simple string comparison functions
            int is_ticks = 1, is_memory = 1, is_help = 1;
            char *ticks_cmd = "ticks", *memory_cmd = "memory", *help_cmd = "help";
            
            // Compare with "ticks"
            for (int i = 0; i < 6; i++) {
                if (result[i] != ticks_cmd[i]) {
                    is_ticks = 0;
                    break;
                }
            }
            if (result[5] != '\0') is_ticks = 0;
            
            // Compare with "memory"
            for (int i = 0; i < 7; i++) {
                if (result[i] != memory_cmd[i]) {
                    is_memory = 0;
                    break;
                }
            }
            if (result[6] != '\0') is_memory = 0;
            
            // Compare with "help"
            for (int i = 0; i < 5; i++) {
                if (result[i] != help_cmd[i]) {
                    is_help = 0;
                    break;
                }
            }
            if (result[4] != '\0') is_help = 0;
            
            if (is_ticks) {
                puts("System ticks: ");
                puts_uint64(ticks);
                puts("\n");
            }
            else if (is_memory) {
                uint64_t total_memory = parse_multiboot2_memory_map(mbi);
                puts("Memory Information:\n");
                puts("  Total usable: ");
                puts_uint64(total_memory);
                puts(" bytes (");
                puts_uint64(total_memory / 1024);
                puts(" KB, ");
                puts_uint64(total_memory / (1024 * 1024));
                puts(" MB)\n");
            }
            else if (is_help) {
                puts("Available commands:\n");
                puts("  ticks  - Show system ticks\n");
                puts("  memory - Show memory information\n");
                puts("  help   - Show this help\n");
            }
            else {
                puts("Unknown command: ");
                puts(result);
                puts("\nType 'help' for available commands\n");
            }
            
            puts("shell> ");
        }
        
        // Increment ticks and small pause
        ticks++;
        for (volatile int i = 0; i < 100000; i++);
    }
}
