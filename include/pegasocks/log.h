#ifndef _PGS_LOG_H
#define _PGS_LOG_H

#include "syslog2.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mpsc.h"


typedef enum { LOG_DEBUG_, LOG_INFO_, LOG_WARN, LOG_ERROR } LOG_LEVEL;

typedef struct pgs_logger_s {
	pgs_mpsc_t *mpsc;
	LOG_LEVEL level;
	uint32_t tid;
	bool isatty;
} pgs_logger_t;

typedef struct pgs_logger_msg_s {
	char *msg;
	uint32_t tid;
} pgs_logger_msg_t;

typedef struct pgs_logger_server_s {
	pgs_logger_t *logger;
	FILE *output;
} pgs_logger_server_t;

#define MAX_MSG_LEN 4096
#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define SESSION_TIME_FORMAT "%H:%M:%S"
#define pgs_logger_debug(logger, ...)                                          \
	syslog2(LOG_DEBUG, __VA_ARGS__)
#define pgs_logger_info(logger, ...)                                           \
	syslog2(LOG_INFO, __VA_ARGS__)
#define pgs_logger_warn(logger, ...)                                           \
	syslog2(LOG_WARNING, __VA_ARGS__)
#define pgs_logger_error(logger, ...)                                          \
	syslog2(LOG_ERR, __VA_ARGS__)
#define pgs_logger_main_info(fp, ...)                                          \
	syslog2(LOG_INFO, __VA_ARGS__)
#define pgs_logger_main_debug(fp, ...)                                         \
	syslog2(LOG_DEBUG, __VA_ARGS__)
#define pgs_logger_main_error(fp, ...)                                         \
	syslog2(LOG_ERR, __VA_ARGS__)
#define pgs_logger_main_warn(fp, ...)                                          \
	syslog2(LOG_WARNING, __VA_ARGS__)

#define PARSE_TIME_NOW(buffer)                                                 \
	do {                                                                   \
		time_t t;                                                      \
		struct tm *now;                                                \
		time(&t);                                                      \
		now = localtime(&t);                                           \
		strftime(buffer, sizeof(buffer), TIME_FORMAT, now);            \
	} while (0)

#define PARSE_SESSION_TIMEVAL(buffer, tv)                                      \
	do {                                                                   \
		struct tm *tm_info;                                            \
		int millisec = tv.tv_usec / 1000.0;                            \
		tm_info = localtime(&tv.tv_sec);                               \
		strftime(buffer, sizeof(buffer), SESSION_TIME_FORMAT,          \
			 tm_info);                                             \
		sprintf(buffer, "%s.%03d", buffer, millisec);                  \
	} while (0)

void pgs_logger_debug_buffer(pgs_logger_t *logger, unsigned char *buf,
			     int size);

pgs_logger_t *pgs_logger_new(pgs_mpsc_t *mpsc, LOG_LEVEL level, bool isatty);
void pgs_logger_free(pgs_logger_t *logger);

// for client, construct and send string to mpsc
void pgs_logger_log(LOG_LEVEL level, pgs_logger_t *logger, const char *fmt,
		    ...);

// for main thread
void pgs_logger_main_log(LOG_LEVEL level, FILE *output, const char *fmt, ...);

// logger thread functions
void pgs_logger_tryrecv(pgs_logger_t *logger, FILE *output);

pgs_logger_msg_t *pgs_logger_msg_new(char *msg, uint32_t tid);
void pgs_logger_msg_free(pgs_logger_msg_t *lmsg);

#endif
