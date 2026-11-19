/*
 * report.c - tee-style report output for router-check.
 */
#include "router-check.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int report_open(rc_report *rep, const char *path)
{
    return zx_output_open(&rep->out, path);
}

void report_close(rc_report *rep)
{
    zx_output_close(&rep->out);
}

void report_line(rc_report *rep, const char *fmt, ...)
{
    char line[4096];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    zx_output_line(&rep->out, "%s", line);
}

void report_text(rc_report *rep, const char *text)
{
    zx_output_text(&rep->out, text);
}

void report_section(rc_report *rep, const char *name)
{
    report_line(rep, "");
    report_line(rep, "== %s ==", name);
}

void report_file(rc_report *rep, const char *path)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        report_line(rep, "cannot read %s: %s", path, strerror(errno));
        return;
    }
    zx_output_stream(&rep->out, fp);
    fclose(fp);
}

size_t report_popen(rc_report *rep, const char *command)
{
    FILE *fp = popen(command, "r");
    size_t bytes;

    if (fp == NULL)
        return 0;
    bytes = zx_output_stream(&rep->out, fp);
    pclose(fp);
    return bytes;
}
