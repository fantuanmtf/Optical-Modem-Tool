#ifndef ZXNETMON_H
#define ZXNETMON_H

#include <stddef.h>
#include <stdio.h>

#define ZXMON_HISTORY 30
#define ZXMON_ROWS 5

typedef struct {
    const char *iface;
    unsigned interval;
    unsigned long long prev_rx;
    unsigned long long prev_tx;
    unsigned long long cur_rx;
    unsigned long long cur_tx;
    unsigned long long rate_rx;
    unsigned long long rate_tx;
    unsigned long long hist_rx[ZXMON_HISTORY];
    unsigned long long hist_tx[ZXMON_HISTORY];
    unsigned long long max_rate;
} zxmon_state;

typedef struct {
    char **names;
    size_t count;
    size_t capacity;
} zxmon_iflist;

/* netdev.c */
int netdev_read(const char *iface, unsigned long long *rx,
                unsigned long long *tx);
void netdev_list(FILE *fp);
void netdev_iflist_free(zxmon_iflist *list);
int netdev_detect_virtual(zxmon_iflist *list);
int netdev_all_nonloopback(zxmon_iflist *list);

/* render.c */
void render_frame(const zxmon_state *state);
void render_multi_frame(const zxmon_state *states, size_t count,
                        const char *mode);

/* ui.c */
void ui_clear_screen(void);
int monitor_run(const char *iface, unsigned interval);
int monitor_run_many(char *const *ifaces, size_t count, unsigned interval,
                     const char *mode);

#endif /* ZXNETMON_H */
