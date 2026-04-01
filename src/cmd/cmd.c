#include "../../inc/cmd.h"
#include "../../inc/mem.h"
//========================
// self_* system call wrapper layer
//========================
int self_printf(const char *format, ...) {
    int ret;
    va_list args;
    va_start(args, format);
    ret = vprintf(format, args);   // currently use standard vprintf
    va_end(args);
    return ret;
}
int self_scanf(const char *format, ...) {
    int ret;
    va_list args;
    va_start(args, format);
    ret = vscanf(format, args);    // currently use standard vscanf
    va_end(args);
    return ret;
}
char *self_fgets(char *buffer, int size) {
    return fgets(buffer, size, stdin); // currently use standard fgets
}
int self_system(const char *command) {
    return system(command);            // currently use standard system
}
//========================
// Simple string utilities used only in this file
//========================
// Remove CR/LF characters in-place
void remove_newline(char *s) {
    int i = 0;
    if (!s) return;
    while (s[i] != '\0') {
        if (s[i] == '\r' || s[i] == '\n') {
            s[i] = '\0';
            break;
        }
        ++i;
    }
}
// Extract first word from src into dst
void get_first_word(const char *src, char *dst, int dstSize) {
    int i = 0;
    int j = 0;
    if (!src || !dst || dstSize <= 0) return;
    // skip leading whitespace
    while (src[i] == ' ' || src[i] == '\t') {
        ++i;
    }
    // copy first word
    while (src[i] != '\0' && src[i] != ' ' && src[i] != '\t') {
        if (j < dstSize - 1) {
            dst[j++] = src[i];
        }
        ++i;
    }
    dst[j] = '\0';
}
// Compare two null-terminated strings for equality
int strings_equal(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        ++i;
    }
    return (a[i] == '\0' && b[i] == '\0');
}
// Skip "echo" keyword and following spaces, return message part
char *get_echo_message(char *cmd) {
    char *p = cmd;
    if (!p) return NULL;
    // skip leading whitespace
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    // skip first word (e.g. "echo")
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        ++p;
    }
    // then skip whitespace, the rest is the argument
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    return p;
}
//========================
// "Kernel" / terminal logic
//========================
void help(void) {
    self_printf("Available commands:\n");
    self_printf("  help       Show help\n");
    self_printf("  clear      Clear screen\n");
    self_printf("  echo ...   Print text\n");
    self_printf("  dir        Show directory\n");
    self_printf("  sysinfo    System information\n");
    self_printf("  meminfo    Memory system status\n");
    self_printf("  exit       Exit terminal\n");
}
void clear(void) {
    self_system("cls");
}
void echo(const char *msg) {
    self_printf("%s\n", msg);
}
void dir(void) {
    self_printf("  Directory: C:\\MiniOS\\\n");
    self_printf("  kernel.sys\n");
    self_printf("  shell.exe\n");
    self_printf("  user.cfg\n");
    self_printf("  boot.ini\n");
}
void sysinfo(void) {
    self_printf("=== System Information ===\n");
    self_printf("OS    : MiniOS 1.0\n");
    self_printf("Build : 2026\n");
    self_printf("Shell : Terminal\n");
    self_printf("Compiler: MSVC (C)\n");
}
void meminfo(void) {
    print_mem_status(); // 调用你写好的函数打印命中率和占用
}
void welcome(void) {
    clear();
    self_printf("========================================\n");
    self_printf("       Mini OS Terminal (C/VS)          \n");
    self_printf("          Type 'help' for commands      \n");
    self_printf("========================================\n");
}

#include "../../inc/file_myfs.h"
#include "../../inc/process_process.h"

static MyFS global_fs;
static int fs_ready = 0;

void os_terminal_init(void) {
    if (myfs_mount(&global_fs, "src/file/mydisk.img") == 0) {
        fs_ready = 1;
    } else {
        self_printf("Warning: Virtual disk 'src/file/mydisk.img' mount failed. File commands may not work.\n");
    }
}

char *get_cmd_arg(char *cmd) {
    char *p = cmd;
    if (!p) return NULL;
    while (*p == ' ' || *p == '\t') ++p;
    while (*p != '\0' && *p != ' ' && *p != '\t') ++p;
    while (*p == ' ' || *p == '\t') ++p;
    return p;
}

void cmd_ls(const char *arg) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    if (arg && *arg) myfs_list_dir(&global_fs, arg);
    else myfs_list_dir(&global_fs, NULL);
}

void cmd_cd(const char *arg) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    if (arg && *arg) myfs_change_dir(&global_fs, arg);
    else myfs_change_dir(&global_fs, "/");
}

void cmd_mkdir(const char *arg) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    if (arg && *arg) myfs_create_dir(&global_fs, arg);
    else self_printf("Usage: mkdir <dir>\n");
}

void cmd_rmdir(const char *arg) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    if (arg && *arg) myfs_delete_dir(&global_fs, arg);
    else self_printf("Usage: rmdir <dir>\n");
}

void cmd_touch(const char *arg) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    if (arg && *arg) myfs_create_file(&global_fs, arg);
    else self_printf("Usage: touch <file>\n");
}

void cmd_cat(const char *arg) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    if (arg && *arg) myfs_read_file(&global_fs, arg);
    else self_printf("Usage: cat <file>\n");
}

void cmd_rm(const char *arg) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    if (arg && *arg) myfs_delete_file(&global_fs, arg);
    else self_printf("Usage: rm <file>\n");
}

void cmd_pwd(void) {
    if (!fs_ready) { self_printf("FS not mounted.\n"); return; }
    myfs_pwd(&global_fs);
}

void cmd_ps(void) {
    print_system_state();
}
