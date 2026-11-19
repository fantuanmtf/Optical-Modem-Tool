/*
 * render.c - draw one zxnetmon screen (plain ANSI, no ncurses).
 */
#include "zxnetmon.h"
#include "zxcommon.h"

#include <stdio.h>

static void human_one(double value, char *buf, size_t buflen)
{
    static const char *const units[] = { "B", "KB", "MB", "GB", "TB" };
    size_t unit = 0;

    while (value >= 1024.0 && unit + 1 < ZX_ARRAY_LEN(units)) {
        value /= 1024.0;
        unit++;
    }
    snprintf(buf, buflen, "%.1f %s", value, units[unit]);
}

static void render_graph(const zxmon_state *state)
{
    unsigned long long max = state->max_rate;
    int row;
    int col;

    if (max == 0)
        max = 1;

    for (row = ZXMON_ROWS; row >= 1; row--) {
        char label[32];

        human_one((double)max * row / ZXMON_ROWS, label, sizeof(label));
        printf("%10s |", label);
        for (col = 0; col < ZXMON_HISTORY; col++) {
            double rx = (double)state->hist_rx[col] * ZXMON_ROWS /
                        (double)max + 0.5;
            double tx = (double)state->hist_tx[col] * ZXMON_ROWS /
                        (double)max + 0.5;
            char cell;

            if ((int)rx >= row && (int)tx >= row)
                cell = '#';
            else if ((int)rx >= row)
                cell = '*';
            else if ((int)tx >= row)
                cell = '+';
            else
                cell = ' ';
            putchar(cell);
        }
        putchar('\n');
    }

    printf("%10s +", "0.0 B/s");
    for (col = 0; col < ZXMON_HISTORY; col++)
        putchar('-');
    putchar('\n');
}

void render_frame(const zxmon_state *state)
{
    char down[32];
    char up[32];
    char total_rx[32];
    char total_tx[32];

    zx_human_rate(state->rate_rx, down, sizeof(down));
    zx_human_rate(state->rate_tx, up, sizeof(up));
    zx_human_bytes(state->cur_rx, total_rx, sizeof(total_rx));
    zx_human_bytes(state->cur_tx, total_tx, sizeof(total_tx));

    ui_clear_screen();
    printf("========================================\n");
    printf("          ZX NETMONITOR v%s\n", ZX_VERSION);
    printf("========================================\n\n");
    printf("Interface: %s\n\n", state->iface);
    printf("Download:   %s\n", down);
    printf("Upload:     %s\n\n", up);
    printf("RX Total:   %s\n", total_rx);
    printf("TX Total:   %s\n\n", total_tx);
    printf("----------------------------------------\n");
    printf("        REALTIME TRAFFIC (last %d sec)\n", ZXMON_HISTORY);
    printf("----------------------------------------\n");
    render_graph(state);
    printf("\nLegend: * RX   + TX   # both\n");
    printf("Press Ctrl+C to exit\n");
    fflush(stdout);
}
