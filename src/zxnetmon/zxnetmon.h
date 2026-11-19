#ifndef ZXNETMON_H
#define ZXNETMON_H

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

/* netdev.c */
int netdev_read(const char *iface, unsigned long long *rx, unsigned long long *tx);
void netdev_list(FILE *fp);

/* render.c */
void render_frame(const zxmon_state *state);

/* ui.c */
void ui_clear_screen(void);
int monitor_run(const char *iface, unsigned interval);

#endif /* ZXNETMON_H */
