#include "../../inc/cmd.h"
#include "../../inc/process_interface.h"
#include "../../inc/process_process.h"
#include "../../inc/platform_time.h"

static char cmd_buf[MAX_CMD];
static int cmd_len = 0;
static int shell_prompt_printed = 0;

void shell_process_entry(void) {
    if (!shell_prompt_printed) {
        self_printf("\n[MiniOS] > ");
        shell_prompt_printed = 1;
    }

    if (self_kbhit()) {
        int ch = self_getch();
        if (ch == '\r' || ch == '\n') {
            self_printf("\n");
            cmd_buf[cmd_len] = '\0';
            
            char op[MAX_CMD] = {0};
            get_first_word(cmd_buf, op, MAX_CMD);
            
            if (strings_equal(op, "help")) { help(); } 
            else if (strings_equal(op, "clear")) { clear(); } 
            else if (strings_equal(op, "echo")) { echo(get_echo_message(cmd_buf)); } 
            else if (strings_equal(op, "ls")) { cmd_ls(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "cd")) { cmd_cd(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "mkdir")) { cmd_mkdir(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "rmdir")) { cmd_rmdir(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "touch")) { cmd_touch(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "cat")) { cmd_cat(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "rm")) { cmd_rm(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "pwd")) { cmd_pwd(); }
            else if (strings_equal(op, "ps")) { cmd_ps(); }
            else if (strings_equal(op, "free")) { meminfo(); }
            else if (strings_equal(op, "memalgo")) { cmd_memalgo(get_cmd_arg(cmd_buf)); }
            else if (strings_equal(op, "exit")) { 
                self_printf("Exiting system...\n"); 
                exit(0); 
            }
            else if (op[0] != '\0') { self_printf("Unsupported command: %s\n", op); }
            
            cmd_len = 0;
            shell_prompt_printed = 0;
        } else if (ch == '\b') { // Backspace
            if (cmd_len > 0) {
                cmd_len--;
                self_printf("\b \b");
            }
        } else {
            if (cmd_len < MAX_CMD - 1) {
                cmd_buf[cmd_len++] = ch;
                self_printf("%c", ch);
            }
        }
    }
}

int main(void) {
    // 1. Init sub-systems
    init_memory_system(8); // Initialize memory with 8 physical pages
    process_manager_init();
    os_terminal_init();
    welcome();
    
    // 2. Wrap Terminal into a Process!
    os_create_process(shell_process_entry);

    // 3. Kernel Main Scheduling Loop (Control Inversion!)
    while (1) {
        os_yield(); // Drive the CPU scheduler to run one time slice
        os_sleep_ms(5);   // Wait 5ms simulating hardware crystal oscillator tick
    }
    return 0;
}
