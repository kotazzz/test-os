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

#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_END 0

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

#define MULTIBOOT_TAG_ALIGN_MASK 7

uint64_t parse_multiboot2_memory_map(void *mbi) {
    if (mbi == NULL) {
        return 0;
    }
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
    return total_usable; // bytes
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
#define MAX_UINT64_DIGITS 21 // Enough for 64-bit number in decimal + null terminator

static void puts_uint64(uint64_t num) {
    char buffer[MAX_UINT64_DIGITS];
    int i = MAX_UINT64_DIGITS - 1;
    buffer[i--] = '\0'; // Null-terminate the string
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
static void display_memory_info(uint64_t total_memory) {
    puts("Total usable memory: ");
    puts_uint64(total_memory);
    puts(" bytes\n");
    puts("Total usable memory: ");
    puts_uint64(total_memory / 1024);
    puts(" KB\n");
    puts("Total usable memory: ");
    puts_uint64(total_memory / (1024 * 1024));
    puts(" MB\n");
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

    uint64_t total_memory =  parse_multiboot2_memory_map(mbi);
    display_memory_info(total_memory);

    // Main loop - just halt and wait for interrupts
    while(1) {
        __asm__ volatile ("hlt");
    }
}
    }
}
