#ifndef SHELL_H
#define SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

// Command handler function type
typedef void (*command_handler_t)(void *mbi);

// Command structure
typedef struct {
    const char *name;
    const char *description;
    command_handler_t handler;
} shell_command_t;

// Initialize and run the mini shell
void run_shell(void *mbi);

#ifdef __cplusplus
}
#endif

#endif // SHELL_H
