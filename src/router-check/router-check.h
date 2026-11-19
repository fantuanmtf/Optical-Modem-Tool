#ifndef ROUTER_CHECK_H
#define ROUTER_CHECK_H

#include "zxcommon.h"

typedef struct {
    zx_output out;
} rc_report;

/* report.c */
int report_open(rc_report *rep, const char *path);
void report_close(rc_report *rep);
void report_line(rc_report *rep, const char *fmt, ...);
void report_text(rc_report *rep, const char *text);
void report_section(rc_report *rep, const char *name);
void report_file(rc_report *rep, const char *path);
size_t report_popen(rc_report *rep, const char *command);

/* system.c */
void rc_system(rc_report *rep);
void rc_cpu(rc_report *rep);
void rc_processes(rc_report *rep);

/* memory.c */
void rc_memory(rc_report *rep);

/* storage.c */
void rc_storage(rc_report *rep);

/* netinfo.c */
void rc_network(rc_report *rep);
void rc_routes(rc_report *rep);
void rc_listening(rc_report *rep);
void rc_bridge(rc_report *rep);
void rc_arp(rc_report *rep);
void rc_wireless(rc_report *rep);

#endif /* ROUTER_CHECK_H */
