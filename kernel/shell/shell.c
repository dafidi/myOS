#include "shell.h"

#include <drivers/keyboard/keyboard_map.h>
#include <drivers/disk/disk.h>
#include <fs/filesystem.h>
#include <kernel/mm.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/system.h>

#define NUM_KNOWN_COMMANDS 8

extern struct fnode root_fnode;
extern struct dir_entry root_dir_entry;

extern void disk_test(void);

volatile int shell_input_counter_ = 0;
volatile int last_processed_pos_ = 0;
static int last_executed_pos_ = 0;
static int ascii_buffer_head_ = 0;
bool processing_input_ = false;

static char execution_bounce_buffer[SHELL_CMD_INPUT_LIMIT];
static char shell_ascii_buffer[SHELL_CMD_INPUT_LIMIT];
char shell_scancode_buffer[SHELL_CMD_INPUT_LIMIT];
static char* known_commands[NUM_KNOWN_COMMANDS] = {
    "hi",
    "ls",
    "pwd",
    "newfi",
    "newfo",
    "disk-test",
    "disk-id",
    "cd"
};
static char prompt[MAX_FILENAME_LENGTH + 3];
static const char stub[3] = "$ ";

static struct fs_context current_fs_ctx = {
    .curr_dir_fnode = &root_fnode,
};

int change_directory(char *target_dir_name) {
    struct directory_chain_link *new_link;
    struct fnode target_dir_fnode;
    int error = 0;

    error = fs_search(current_fs_ctx.working_directory_chain, target_dir_name, &target_dir_fnode);
    if (error) {
        print_string("Can't change directories to "); print_string(target_dir_name); print_string("\n");
        return -1;
    }

    new_link = (struct directory_chain_link *) object_alloc(sizeof(struct directory_chain_link));
    new_link->id = target_dir_fnode.id;

    current_fs_ctx.working_directory_chain->tail->next = new_link;
    current_fs_ctx.working_directory_chain->tail = new_link;

    clear_buffer((uint8_t *) prompt, MAX_FILENAME_LENGTH + 3);
    memory_copy(target_dir_name, prompt, strlen(target_dir_name));
    memory_copy(stub, prompt + strlen(target_dir_name), 2);
    return error;
}

static void exec_known_cmd(const int cmd) {
    switch (cmd) {
    case 0:
        print_string("hi to you too!\n");
        break;
    case 1: {
        if (list_dir_content(current_fs_ctx.working_directory_chain)) {
            print_string("Error showing dir with current chain.\n");
            return;
        }

        break;
    }
    case 2: {
        struct dir_info dir_info;

        if (get_dir_info(current_fs_ctx.curr_dir_fnode, &dir_info)) {
            print_string("Failed to get dir_info for [fnode=");
            print_ptr(current_fs_ctx.curr_dir_fnode);
            print_string("]\n");
            return;
        }
        print_string("The current directory is: [");
        print_string(dir_info.name);
        print_string("].\n");
        break;
    }
    case 3: {
        char text[] = "Bien Venue!";
        struct file_creation_info info = {
            .name = "new_file",
            .file_size = strlen(text),
            .file_content = (uint8_t *) text
        };

        print_string("One new file coming right up!\n");
        if (create_file(&current_fs_ctx, &info))
            print_string("Sorry, file creation failed.\n");
        break;
    }
    case 4: {
        struct folder_creation_info info = {
            .name = "new_folder",
        };

        print_string("One new folder coming up!\n");
        if (create_folder(&current_fs_ctx, &info))
            print_string("Sorry, folder creation failed.\n");
        break;
    }
    case 5: {
        print_string("Running disk_test.\n");
        disk_test();

        break;
    }
    case 6: {
        print_string("Running \"IDENTIFY DEVICE\" ATA command.\n");
        identify_device();

        break;
    }
    case 7: {
        char dir_name[MAX_FILENAME_LENGTH] = "new_folder";

        clear_buffer((uint8_t *)dir_name + strlen(dir_name), MAX_FILENAME_LENGTH - strlen(dir_name));
        if (change_directory(dir_name))
            print_string("Error: unable to change directory.\n");

        break;
    }
    default:
        print_string("don't know what that is sorry :(\n");
    }
}

static void exec(char* input) {
    int i, l, m;

    print_string("Attempting to execute: ["); print_string(input); print_string("]\n");

    m = strlen(input);
    for (i = 0; i < NUM_KNOWN_COMMANDS; i++) {
        l = strlen(known_commands[i]);

        if (l != m)
            continue;

        if (strmatchn(known_commands[i], input, l))
            break;
    }

    if (i >= NUM_KNOWN_COMMANDS)
        print_string("Sorry, can't help with that... yet!\n");

    if (i < NUM_KNOWN_COMMANDS)
        exec_known_cmd(i);
}

void reset_shell_counters(void) {
    shell_input_counter_ = 0;
    last_processed_pos_ = 0;
    last_executed_pos_ = 0;
    ascii_buffer_head_ = 0;
}

static bool process_new_scancodes(const int offset, const int num_new_characters) {
    bool reshow_prompt = false;

    for (int i = 0; i < num_new_characters; i++, last_processed_pos_++) {
        uint8_t scancode = shell_scancode_buffer[offset + i];
        char ascii_char = US_KEYBOARD_MAP[scancode];
        char char_buff[2] = { ascii_char, '\0' };

        if ((scancode & 0x80) || !scancode)
            continue;

        print_string(char_buff);

        if (ascii_char == '\n') {
            int e, p;

            reshow_prompt = true;
            if (last_executed_pos_ == ascii_buffer_head_)
                continue;

            e = last_executed_pos_ % SHELL_CMD_INPUT_LIMIT;
            p = ascii_buffer_head_ % SHELL_CMD_INPUT_LIMIT;

            if (e < p) {
                memory_copy(&shell_ascii_buffer[e], execution_bounce_buffer, p - e);
            } else {
                memory_copy(&shell_ascii_buffer[e], execution_bounce_buffer, SHELL_CMD_INPUT_LIMIT - e);
                memory_copy(&shell_ascii_buffer[0], &execution_bounce_buffer[e], p);
            }

            exec(execution_bounce_buffer);

            clear_buffer((uint8_t *) execution_bounce_buffer, SHELL_CMD_INPUT_LIMIT);
            last_executed_pos_ = ascii_buffer_head_;
        } else {
            int write_pos = ascii_buffer_head_++ % SHELL_CMD_INPUT_LIMIT;

            shell_ascii_buffer[write_pos] = ascii_char;
        }
    }

    return reshow_prompt;
}

static void show_prompt(void) {
    print_string(prompt);
}

void main_shell_init(void) {
    current_fs_ctx.curr_dir_fnode_location = root_dir_entry.fnode_location;
    current_fs_ctx.curr_dir_fnode = &root_fnode;
    current_fs_ctx.working_directory_chain = (struct directory_chain *) object_alloc(sizeof(struct directory_chain));
    current_fs_ctx.working_directory_chain->head = (struct directory_chain_link *) object_alloc(sizeof(struct directory_chain_link));
    current_fs_ctx.working_directory_chain->head->id = root_fnode.id;
    current_fs_ctx.working_directory_chain->head->next = NULL;
    current_fs_ctx.working_directory_chain->head->prev = NULL;
    current_fs_ctx.working_directory_chain->tail = current_fs_ctx.working_directory_chain->head;

    memory_copy((char *)&root_dir_entry.name, prompt, strlen(root_dir_entry.name));
    memory_copy(stub, prompt + strlen(root_dir_entry.name), 2);
}

void main_shell_run(void) {
    bool prompt = true;

    print_string("Main shell Executing.\n");
    shell_input_counter_ = 0;

    while (true) {
        int head = shell_input_counter_ % SHELL_CMD_INPUT_LIMIT;
        int tail = last_processed_pos_ % SHELL_CMD_INPUT_LIMIT;

        // Show shell prompt, "<directory name>$".
        if (prompt)
            show_prompt();

        // Wait for input from keyboard.
        while (tail == head)
            head = shell_input_counter_  % SHELL_CMD_INPUT_LIMIT;

        prompt = process_new_scancodes(tail, head - tail);
    }

    print_string("We should never reach here. Going into infinite loop.\n");

    PAUSE();
}

void exec_main_shell(void) {
    main_shell_init();

    main_shell_run();
}
