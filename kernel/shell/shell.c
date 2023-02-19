#include "shell.h"

#include <drivers/keyboard/keyboard_map.h>
#include <drivers/disk/disk.h>
#include <fs/filesystem.h>
#include <kernel/mm.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/system.h>

#define NUM_KNOWN_COMMANDS 10

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
    "cd",
    "fidel",
    "fodel"
};
static char prompt[MAX_FILENAME_LENGTH + 3];
static char stub[3] = "$ ";

static struct fs_context current_fs_ctx = {
    .curr_dir_fnode = &root_fnode,
};

void update_prompt(void) {
    struct directory_chain_link *chainp;
    char *promptp;
    int len;

    chainp = current_fs_ctx.working_directory_chain->head;
    promptp = prompt;

    while (chainp) {
        len = strlen(chainp->name);

        memory_copy((char *)chainp->name, promptp, len);

        promptp += len;

        *promptp++ = '/';

        chainp = chainp->next;
    }

    memory_copy((char *) stub, promptp, 3);

    promptp += strlen(stub);

    *promptp = '\0';
}

int change_directory(char *path) {
    struct directory_chain *chain;

    chain = create_chain_from_path(&current_fs_ctx, path);
    if (!chain) {
        print_string("cdp failed: bad path?\n");
        return -1;
    }

    destroy_directory_chain(current_fs_ctx.working_directory_chain);
    current_fs_ctx.working_directory_chain = chain;

    update_prompt();

    return 0;
}

static void exec_known_cmd(const int cmd, char *cmdp, char *argsp) {
    switch (cmd) {
    case 0: // hi
        print_string("hi to you too!\n");
        break;
    case 1: { // ls
        char *path = NULL;

        if (strlen(argsp) > 0)
            path = argsp;

        if (list_dir_content(&current_fs_ctx, path)) {
            print_string("Error showing dir with current chain.\n");
            return;
        }

        break;
    }
    case 2: { // pwd
       struct directory_chain_link *link = current_fs_ctx.working_directory_chain->head;

        while (link) {
            print_string(link->name);
            print_string("/");
            link = link->next;
        }
        print_string("\n");

        break;
    }
    case 3: { // newfi
        char text[] = "Bien Venue!";

        if (strlen(argsp) == 0) {
            print_string("Error [newfi]: provide file name.\n");
            break;
        }

        struct file_creation_info info = {
            .file_size = strlen(text),
            .file_content = (uint8_t *) text
        };

        clear_buffer((uint8_t *)info.path, MAX_FILENAME_LENGTH);
        memory_copy((char *)argsp, info.path, strlen(argsp));

        print_string("One new file coming right up!\n");
        if (create_file(&current_fs_ctx, &info))
            print_string("Sorry, file creation failed.\n");
        break;
    }
    case 4: { // newfo
        if (strlen(argsp) == 0) {
            print_string("Error [newfo]: provide new folder name.\n");
            break;
        }

        struct folder_creation_info info;
        clear_buffer((uint8_t *)info.path, MAX_FILENAME_LENGTH);
        memory_copy((char*)argsp, info.path, strlen(argsp));

        print_string("One new folder coming up!\n");
        if (create_folder(&current_fs_ctx, &info))
            print_string("Sorry, folder creation failed.\n");
        break;
    }
    case 5: { // disk-test
        print_string("Running disk_test.\n");
        disk_test();

        break;
    }
    case 6: { // disk-id
        print_string("Running \"IDENTIFY DEVICE\" ATA command.\n");
        identify_device();

        break;
    }
    case 7: { // cd
        char *path = argsp;

        if (change_directory(path))
            print_string("Error: unable to change directory.\n");

        break;
    }
    case 8: { // fidel
        char filename[MAX_FILENAME_LENGTH];

        clear_buffer((uint8_t *) filename, MAX_FILENAME_LENGTH);
        memory_copy((char*)argsp, filename, strlen(argsp));
        if (delete_file(&current_fs_ctx, filename)) {
            print_string("Error: deleting ["); print_string(filename);
            print_string("] failed.\n");
        }
        break;
    }
    case 9: {// fodel
        char foldername[MAX_FILENAME_LENGTH];

        clear_buffer((uint8_t *) foldername, MAX_FILENAME_LENGTH);
        memory_copy((char*)argsp, foldername, strlen(argsp));
        if (delete_folder(&current_fs_ctx, foldername)) {
            print_string("Error: deleting ["); print_string(foldername);
            print_string("] failed.\n");
        }
        break;
    }
    default:
        print_string("don't know what that is sorry :(\n");
    }
}

static void exec(char* input) {
    int i, l, m;
    char *cmdp = input, *argsp;

    print_string("Attempting to execute: ["); print_string(input); print_string("]\n");

    // Ignore leading spaces.
    while (*cmdp == ' ')
        cmdp++;

    argsp = cmdp;

    // Go past command characters.
    while (*argsp != ' ' && *argsp != '\0')
        argsp++;

    if (*argsp == ' ')
        *argsp++ = '\0';

    // Go past spaces after command but before args.
    while (*argsp == ' ' && *argsp != '\0')
        argsp++;

    m = strlen(cmdp);
    for (i = 0; i < NUM_KNOWN_COMMANDS; i++) {
        l = strlen(known_commands[i]);

        if (l != m)
            continue;

        if (strmatchn(known_commands[i], cmdp, l))
            break;
    }

    if (i >= NUM_KNOWN_COMMANDS)
        print_string("Sorry, can't help with that... yet!\n");

    print_string("cmd=["); print_string(cmdp); print_string("]");
    print_string(" args=["); print_string(argsp); print_string("]\n");
    if (i < NUM_KNOWN_COMMANDS)
        exec_known_cmd(i, cmdp, argsp);
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

            // This could happen if a bunch of '\n's are entered.
            if (last_executed_pos_ == ascii_buffer_head_)
                continue;

            e = last_executed_pos_ % SHELL_CMD_INPUT_LIMIT;
            p = ascii_buffer_head_ % SHELL_CMD_INPUT_LIMIT;

            if (e < p) {
                memory_copy((char*)&shell_ascii_buffer[e], execution_bounce_buffer, p - e);
            } else {
                memory_copy((char*)&shell_ascii_buffer[e], execution_bounce_buffer, SHELL_CMD_INPUT_LIMIT - e);
                memory_copy((char*)&shell_ascii_buffer[0], &execution_bounce_buffer[e], p);
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
    current_fs_ctx.working_directory_chain = init_directory_chain();

    memory_copy((char *)&root_dir_entry.name, prompt, strlen(root_dir_entry.name));
    memory_copy((char*)stub, prompt + strlen(root_dir_entry.name), 2);
}

void main_shell_run(void) {
    bool prompt = true;

    print_string("Main shell Executing.\n");
    shell_input_counter_ = 0;

    while (true) {
        int p = last_processed_pos_, c = shell_input_counter_;
        int p_mod = p % SHELL_CMD_INPUT_LIMIT;

        // Show shell prompt, "<directory name>$".
        if (prompt)
            show_prompt();

        // Wait for input from keyboard.
        while (p == c)
            c = shell_input_counter_;

        prompt = process_new_scancodes(p_mod, c - p);
    }

    print_string("We should never reach here. Going into infinite loop.\n");

    PAUSE();
}

void exec_main_shell(void) {
    main_shell_init();

    main_shell_run();
}
