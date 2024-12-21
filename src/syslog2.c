#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef SYSLOG2_COLORS
#define SYSLOG2_COLORS 1
#define COLOR_RESET "\033[0m" // Reset color
#define COLOR_RED "\033[31m" // Red
#define COLOR_GREEN "\033[32m" // Green
#define COLOR_YELLOW "\033[33m" // Yellow
#define COLOR_BLUE "\033[34m" // Blue
#define COLOR_BLUE_ON_WHITE "\033[34;47m"
#define COLOR_BLUE_BRIGHT "\033[94m"
#endif

#include <pthread.h> // pthread_spinlock_t, pthread_spin_lock, pthread_spin_unlock
#include <stdarg.h> // va_list, va_start(), va_end()
#include <stdatomic.h>
#include <stdio.h> // printf()
#include <stdlib.h>
#include <sys/syscall.h> // SYS_gettid
#include <sys/time.h> // gettimeofday()
#include <syslog.h> // syslog()
#include <time.h> // localtime(), struct tm
#include <unistd.h> // syscall()

#include "syslog.h"

static const char *priority_texts[] = { "EMERG", "ALERT", "CRIT ", "ERROR",
					"WARN ", "NOTIC", "INFO ", "DEBUG" };

static const char *strprio(int pri)
{
	if (pri >= LOG_EMERG && pri <= LOG_DEBUG) {
		return priority_texts[pri];
	} else {
		return "UNKNOWN";
	}
}

// Spinlock for synchronizing threads
static pthread_spinlock_t spinlock;

static atomic_int initialized = 0;
static time_t offset_seconds;
static long timezone_offset;

void initialize_time_cache()
{
	if (atomic_exchange(&initialized, 1) == 0) {
		if (pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE) !=
		    0) {
			perror("Failed to initialize spinlock");
			exit(EXIT_FAILURE);
		}
		struct timespec startup_time_realtime;
		struct timespec startup_time_monotonic;
		clock_gettime(CLOCK_REALTIME, &startup_time_realtime);
		clock_gettime(CLOCK_MONOTONIC, &startup_time_monotonic);

		// calculate offset between CLOCK_REALTIME and CLOCK_MONOTONIC in seconds
		offset_seconds = startup_time_realtime.tv_sec -
				 startup_time_monotonic.tv_sec;

		// Get localtime offset
		struct tm local_tm;
		localtime_r(&startup_time_realtime.tv_sec, &local_tm);
		timezone_offset = local_tm.tm_gmtoff;
	}
}

void get_current_time(struct timespec *ts, struct tm *tm_info)
{
	if (!atomic_load(&initialized)) {
		initialize_time_cache();
	}

	clock_gettime(CLOCK_MONOTONIC, ts);

	time_t current_timestamp = offset_seconds + ts->tv_sec;

	// add cached localtime offset
	current_timestamp += timezone_offset;

	// convert to broken-down time representation
	gmtime_r(&current_timestamp, tm_info);
}

void syslog2_(int pri, const char *filename, int line, const char *fmt, ...)
{
	char priobuf[32];
	switch (pri) {
	case LOG_EMERG:
		//pass
	case LOG_ALERT:
		//pass
	case LOG_CRIT:
		//pass
	case LOG_ERR:
		snprintf(priobuf, sizeof(priobuf), COLOR_RED "%s" COLOR_RESET,
			 strprio(pri));
		break;
	case LOG_WARNING:
		snprintf(priobuf, sizeof(priobuf),
			 COLOR_YELLOW "%s" COLOR_RESET, strprio(pri));
		break;
	case LOG_NOTICE:
		snprintf(priobuf, sizeof(priobuf), COLOR_GREEN "%s" COLOR_RESET,
			 strprio(pri));
		break;
	case LOG_INFO:
		snprintf(priobuf, sizeof(priobuf),
			 COLOR_BLUE_BRIGHT "%s" COLOR_RESET, strprio(pri));
		break;
	case LOG_DEBUG:
		//pass
	defaut:
		snprintf(priobuf, sizeof(priobuf), "%s", strprio(pri));
		break;
	}

	if (!atomic_load(&initialized)) {
		initialize_time_cache();
	}

	int run_flag = setlogmask(0) & LOG_MASK(pri);
	if (!run_flag)
		return;

	static char msg[32768];
	static char timebuf[128];

	struct timespec ts;
	struct tm tm_info;
	// printf("before spinlock\n");
	pthread_spin_lock(&spinlock); // Lock the spinlock

	get_current_time(&ts, &tm_info);

	snprintf(timebuf, sizeof(timebuf),
		 "%02d-%02d-%02d %02d:%02d:%02d.%09ld", tm_info.tm_mday,
		 tm_info.tm_mon + 1, tm_info.tm_year + 1900, tm_info.tm_hour,
		 tm_info.tm_min, tm_info.tm_sec, ts.tv_nsec);

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	syslog(pri, "[%ld] %s:%d: %s", syscall(SYS_gettid), filename, line,
	       msg);
	printf("[%s] %s [%ld] %s:%d: %s\n", timebuf, priobuf,
	       syscall(SYS_gettid), filename, line, msg);

	pthread_spin_unlock(&spinlock); // Unlock the spinlock
	// printf("release spinlock\n");
}

void syslog2_printf_(int pri, const char *filename, int line, const char *fmt,
		     ...)
{
	int run_flag = setlogmask(0) & LOG_MASK(pri);
	if (!run_flag)
		return;

	va_list args;
	va_start(args, fmt);

	// Print to stdout
	vprintf(fmt, args);

	// Reset va_list for syslog
	va_end(args);
	va_start(args, fmt);
	// printf("before spinlock\n");
	pthread_spin_lock(&spinlock); // Lock the spinlock

	// Print to syslog
	vsyslog(pri, fmt, args);

	va_end(args);
	pthread_spin_unlock(&spinlock); // Unlock the spinlock
	// printf("release spinlock\n");
}