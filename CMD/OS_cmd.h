// Public interfaces and constants for OS_cmd

#pragma once

// Basic headers (simulated OS + C standard library)
#include <stdio.h>
#include <stdlib.h>


#define MAX_CMD 1024


//========================
// self_* "system call" interface layer
//========================

// I/O interfaces: upper layer only calls self_*, internally uses C library
int  self_printf(const char *format, ...);
int  self_scanf(const char *format, ...);
char *self_fgets(char *buffer, int size);
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
void welcome(void);

