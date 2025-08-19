#include <stdint.h>

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

// Function declarations
static void puts(const char* s);
static void putc(char c);
static void puts_uint64(uint64_t num);
static void puts_hex64(uint64_t num);

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


// IDT structures
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_pointer;

static volatile uint16_t* VGA = (uint16_t*)0xB8000;
static int row=0, col=0;

// Keyboard scancode to ASCII table (US layout)
static char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// I/O port functions
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a" (val), "dN" (port));
}

static void putc(char c){
    if(c == '\n'){ 
        col = 0; 
        row++; 
        if(row >= 25) {
            // Scroll screen up
            for(int i = 0; i < 80 * 24; i++) {
                VGA[i] = VGA[i + 80];
            }
            // Clear last line
            for(int i = 80 * 24; i < 80 * 25; i++) {
                VGA[i] = ' ' | (0x07 << 8);
            }
            row = 24;
        }
        return; 
    }
    if(c == '\b'){
        if(col > 0) {
            col--;
            VGA[row * 80 + col] = ' ' | (0x07 << 8);
        }
        return;
    }
    
    VGA[row * 80 + col] = (uint16_t)c | (0x07 << 8);
    if(++col >= 80){ 
        col = 0; 
        row++; 
        if(row >= 25) {
            // Scroll screen up
            for(int i = 0; i < 80 * 24; i++) {
                VGA[i] = VGA[i + 80];
            }
            // Clear last line
            for(int i = 80 * 24; i < 80 * 25; i++) {
                VGA[i] = ' ' | (0x07 << 8);
            }
            row = 24;
        }
    }
}

static void puts(const char* s){
    while(*s) putc(*s++);
}

static void puts_uint64(uint64_t num) {
    char buffer[21]; // Enough for 64-bit number in decimal + null terminator
    int i = 20;
    buffer[i--] = '\0'; // Null-terminate the string

    if (num == 0) {
        buffer[i--] = '0';
    } else {
        while (num > 0) {
            buffer[i--] = '0' + (num % 10);
            num /= 10;
        }
    }

    puts(&buffer[i + 1]); // Print the string starting from the first digit
}

static void puts_hex64(uint64_t num) {
    char buffer[17]; // 16 hex digits + null terminator
    int i = 16;
    buffer[i--] = '\0';
    
    if (num == 0) {
        buffer[i--] = '0';
    } else {
        while (num > 0) {
            int digit = num & 0xF;
            if (digit < 10) {
                buffer[i--] = '0' + digit;
            } else {
                buffer[i--] = 'A' + (digit - 10);
            }
            num >>= 4;
        }
    }
    
    puts(&buffer[i + 1]);
}

// External assembly interrupt handler
extern void keyboard_handler_asm(void);

// C keyboard interrupt handler
void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Only handle key press events (bit 7 = 0)
    if (!(scancode & 0x80)) {
        if (scancode < sizeof(scancode_to_ascii)) {
            char c = scancode_to_ascii[scancode];
            if (c != 0) {
                putc(c);
            }
        }
    }
    
    // Send EOI to PIC
    outb(PIC1_COMMAND, 0x20);
}

// Set IDT entry
static void idt_set_gate(int num, uint64_t handler, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

// Initialize PIC
static void init_pic(void) {
    // ICW1: Initialize PIC
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    
    // ICW2: Set interrupt vector offset
    outb(PIC1_DATA, 0x20); // PIC1 starts at interrupt 32
    outb(PIC2_DATA, 0x28); // PIC2 starts at interrupt 40
    
    // ICW3: Set up cascading
    outb(PIC1_DATA, 0x04); // PIC1 has slave at IRQ2
    outb(PIC2_DATA, 0x02); // PIC2 is slave at IRQ2
    
    // ICW4: Set mode
    outb(PIC1_DATA, 0x01); // 8086 mode
    outb(PIC2_DATA, 0x01); // 8086 mode
    
    // Mask all interrupts except keyboard (IRQ1)
    outb(PIC1_DATA, 0xFD); // Enable IRQ1 (keyboard)
    outb(PIC2_DATA, 0xFF); // Mask all IRQ8-15
}

// Initialize IDT
static void init_idt(void) {
    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = (uint64_t)&idt;
    
    // Clear IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Set keyboard interrupt (IRQ1 = interrupt 33)
    idt_set_gate(33, (uint64_t)keyboard_handler_asm, 0x08, 0x8E);
    
    // Load IDT
    __asm__ volatile ("lidt %0" : : "m" (idt_pointer));
}

void kmain(void *mbi){
    row = 0;
    col = 0;
    
    // Clear screen
    for(int i = 0; i < 80 * 25; i++) 
        VGA[i] = (' ' | (0x07 << 8));
    
    puts("TOS - Tiny Operating System\n");
    puts("64-bit kernel with keyboard support\n");
    puts("Type anything to see echo...\n\n");
    
    // Initialize interrupts
    init_idt();
    init_pic();
    
    // Enable interrupts
    __asm__ volatile ("sti");

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

    // Main loop - just halt and wait for interrupts
    while(1) {
        __asm__ volatile ("hlt");
    }
}
