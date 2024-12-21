#ifndef SYSLOG2_H_
#define SYSLOG2_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>			 // va_list, va_start(), va_end()
#include <stdio.h>			 // printf()
#include <sys/syscall.h> // SYS_gettid
#include <syslog.h>			 // syslog()
#include <unistd.h>			 // syscall()

void syslog2_(int pri, const char *filename, int line, const char *fmt, ...);
void syslog2_printf_(int pri, const char *filename, int line, const char *fmt, ...);

#ifndef syslog2
#define syslog2(pri, fmt, ...) syslog2_(pri, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif // syslog2

#ifndef syslog2_printf
#define syslog2_printf(pri, fmt, ...) syslog2_printf_(pri, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif // syslog2_printf

#endif /* SYSLOG2_H_ */
