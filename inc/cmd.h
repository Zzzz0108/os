// Public interfaces and constants for OS_cmd
#pragma once
// Basic headers (simulated OS + C standard library)
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define MAX_CMD 1024
//========================
// self_* "system call" interface layer
//========================
// I/O interfaces: upper layer only calls self_*, internally uses C library
int  self_printf(const char *format, ...);
int  self_scanf(const char *format, ...);
char *self_fgets(char *buffer, int size);
int  self_kbhit(void);
int  self_getch(void);
int  self_system(const char *command);
// String / command parsing helper functions
void   remove_newline(char *s);
void   get_first_word(const char *src, char *dst, int dstSize);
int    strings_equal(const char *a, const char *b);
char  *get_echo_message(char *cmd);
// Terminal command related interfaces
void help(void);
void clear(void);
void echo(const char *msg);
void dir(void);
void sysinfo(void);
void meminfo(void);
void welcome(void);

// Linux-style commands
void os_terminal_init(void);
void cmd_ls(const char *arg);
void cmd_cd(const char *arg);
void cmd_mkdir(const char *arg);
void cmd_rmdir(const char *arg);
void cmd_touch(const char *arg);
void cmd_cat(const char *arg);
void cmd_rm(const char *arg);
void cmd_pwd(void);
void cmd_ps(void);
char* get_cmd_arg(char *cmd);
