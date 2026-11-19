/*
 * memory.c - /proc/meminfo summary.
 */
#include "router-check.h"

#include <stdio.h>
#include <string.h>

struct mem_value {
    const char *key;
    unsigned long long value;
    char unit[16];
    int seen;
};

void rc_memory(rc_report *rep)
{
    static const char *const keys[] = {
        "MemTotal", "MemFree", "MemAvailable", "Buffers", "Cached",
        "SwapTotal", "SwapFree"
    };
    struct mem_value values[ZX_ARRAY_LEN(keys)];
    FILE *fp;
    char line[256];
    int any = 0;
    size_t i;
    unsigned long long total = 0;
    unsigned long long available = 0;
    unsigned long long free_mem = 0;
    int have_total = 0;
    int have_available = 0;
    int have_free = 0;

    report_section(rep, "Memory");
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        report_line(rep, "no /proc/meminfo");
        return;
    }

    memset(values, 0, sizeof(values));
    for (i = 0; i < ZX_ARRAY_LEN(keys); i++)
        values[i].key = keys[i];

    while (fgets(line, sizeof(line), fp) != NULL) {
        char key[64];
        unsigned long long value;
        char unit[16];

        if (sscanf(line, "%63[^:]: %llu %15s", key, &value, unit) != 3)
            continue;
        for (i = 0; i < ZX_ARRAY_LEN(values); i++) {
            if (strcmp(key, values[i].key) == 0) {
                values[i].value = value;
                snprintf(values[i].unit, sizeof(values[i].unit), "%s", unit);
                values[i].seen = 1;
                any = 1;
                break;
            }
        }
        if (strcmp(key, "MemTotal") == 0) {
            total = value;
            have_total = 1;
        } else if (strcmp(key, "MemAvailable") == 0) {
            available = value;
            have_available = 1;
        } else if (strcmp(key, "MemFree") == 0) {
            free_mem = value;
            have_free = 1;
        }
    }
    fclose(fp);

    if (!any) {
        report_line(rep, "no memory data");
        return;
    }

    for (i = 0; i < ZX_ARRAY_LEN(values); i++) {
        if (values[i].seen) {
            report_line(rep, "%-12s %8llu %s", values[i].key,
                        values[i].value, values[i].unit);
        }
    }

    if (have_total && total > 0 && (have_available || have_free)) {
        unsigned long long usable = have_available ? available :
                                    (have_free ? free_mem : 0);
        double percent;

        if (usable > total)
            usable = total;
        percent = (double)(total - usable) * 100.0 / (double)total;
        report_line(rep, "Used (approx): %.1f%%", percent);
    }
}
