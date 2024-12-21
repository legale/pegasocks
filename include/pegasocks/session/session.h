#ifndef _PGS_SESSION_H
#define _PGS_SESSION_H

#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <event2/event.h>

#include "server/local.h"
#include "inbound.h"
#include "outbound.h"
#include "utils.h"
#include "syslog2.h"

#define pgs_session_debug(session, ...)                                        \
	syslog2(LOG_DEBUG, __VA_ARGS__)
#define pgs_session_info(session, ...)                                         \
	syslog2(LOG_INFO, __VA_ARGS__)
#define pgs_session_warn(session, ...)                                         \
	syslog2(LOG_WARN, __VA_ARGS__)
#define pgs_session_error(session, ...)                                        \
	syslog2(LOG_ERR, __VA_ARGS__)
#define pgs_session_debug_buffer(session, buf, len)                            \
	pgs_logger_debug_buffer(session->local_server->logger, buf, len)

#define PGS_FREE_SESSION(session)                                              \
	pgs_list_del(session->local_server->sessions, session->node)

typedef struct pgs_server_session_stats_s {
	struct timeval start;
	struct timeval end;
	uint64_t send;
	uint64_t recv;
} pgs_session_stats_t;

typedef struct pgs_session_s {
	pgs_session_inbound_t *inbound;
	pgs_session_outbound_t *outbound;
	pgs_local_server_t *local_server;
	pgs_session_stats_t *metrics;

	pgs_list_node_t *node; /* store the value to sessions */
} pgs_session_t;

// session
pgs_session_t *pgs_session_new(int fd, pgs_local_server_t *local_server);
void pgs_session_start(pgs_session_t *session);
void pgs_session_free(pgs_session_t *session);

// metrics
void on_session_metrics_recv(pgs_session_t *session, uint64_t len);
void on_session_metrics_send(pgs_session_t *session, uint64_t len);

#endif
