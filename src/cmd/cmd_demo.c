﻿#include "../../inc/cmd.h"
int main(void) {
    char cmd[MAX_CMD];
    welcome();
    while (1) {
        self_printf("\n[MiniOS] > ");
        if (!self_fgets(cmd, MAX_CMD))
            break;
        // remove trailing newline
        remove_newline(cmd);
        // extract first word
        char op[MAX_CMD] = {0};
        get_first_word(cmd, op, MAX_CMD);
        if (strings_equal(op, "help")) {
            help();
        } else if (strings_equal(op, "clear")) {
            clear();
        } else if (strings_equal(op, "echo")) {
            char *msg = get_echo_message(cmd);
            echo(msg);
        } else if (strings_equal(op, "dir")) {
            dir();
        } else if (strings_equal(op, "sysinfo")) {
            sysinfo();
        } else if (strings_equal(op, "exit")) {
            self_printf("Exiting terminal...\n");
            break;
        } else if (op[0] != '\0') {
            self_printf("Unsupported command: %s\n", op);
        }
    }
    return 0;
}
