/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */

#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

void itoa(int a, char *b);

int strlen(char *a);

void exit();

int fork();

int read(int fd, char *buf, int count);

int write(int fd, char *buffer, int size);

int gettime();

int clone(void (*function) (void), void *stack);

int getpid();

int get_stats(int pid, struct stats *st);

void *sbrk(int increment);

void perror();

#endif  /* __LIBC_H__ */
