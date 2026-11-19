/*
 * ui.c - terminal lifecycle, signal handling and the sampling loop.
 */
#include "zxnetmon.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;
static int cursor_hidden = 0;

static void on_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

void ui_clear_screen(void)
{
    fputs("\033[H\033[2J", stdout);
}

static void hide_cursor(void)
{
    fputs("\033[?25l", stdout);
    fflush(stdout);
    cursor_hidden = 1;
}

static void show_cursor(void)
{
    if (cursor_hidden) {
        fputs("\033[?25h", stdout);
        fflush(stdout);
        cursor_hidden = 0;
    }
}

static unsigned long long update_max(const zxmon_state *state)
{
    unsigned long long max = 1;
    int i;

    for (i = 0; i < ZXMON_HISTORY; i++) {
        if (state->hist_rx[i] > max)
            max = state->hist_rx[i];
        if (state->hist_tx[i] > max)
            max = state->hist_tx[i];
    }
    return max;
}

static void push_history(zxmon_state *state, unsigned long long rx,
                         unsigned long long tx)
{
    int i;

    for (i = 0; i < ZXMON_HISTORY - 1; i++) {
        state->hist_rx[i] = state->hist_rx[i + 1];
        state->hist_tx[i] = state->hist_tx[i + 1];
    }
    state->hist_rx[ZXMON_HISTORY - 1] = rx;
    state->hist_tx[ZXMON_HISTORY - 1] = tx;
}

int monitor_run(const char *iface, unsigned interval)
{
    zxmon_state state;
    struct sigaction action;
    unsigned long long rx;
    unsigned long long tx;
    int i;

    if (netdev_read(iface, &rx, &tx) != 0) {
        fprintf(stderr, "zxnetmon: cannot read counters for '%s'\n", iface);
        return 1;
    }

    for (i = 0; i < ZXMON_HISTORY; i++) {
        state.hist_rx[i] = 0;
        state.hist_tx[i] = 0;
    }
    state.iface = iface;
    state.interval = interval;
    state.prev_rx = rx;
    state.prev_tx = tx;
    state.cur_rx = rx;
    state.cur_tx = tx;
    state.rate_rx = 0;
    state.rate_tx = 0;
    state.max_rate = 1;

    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    hide_cursor();
    render_frame(&state);

    while (!stop_requested) {
        unsigned long long drx;
        unsigned long long dtx;

        sleep(interval);
        if (stop_requested)
            break;

        if (netdev_read(iface, &rx, &tx) != 0) {
            show_cursor();
            fprintf(stderr, "\nzxnetmon: interface '%s' disappeared\n", iface);
            return 1;
        }

        drx = rx >= state.prev_rx ? rx - state.prev_rx : 0;
        dtx = tx >= state.prev_tx ? tx - state.prev_tx : 0;
        state.rate_rx = drx / interval;
        state.rate_tx = dtx / interval;
        state.cur_rx = rx;
        state.cur_tx = tx;
        push_history(&state, state.rate_rx, state.rate_tx);
        state.max_rate = update_max(&state);
        render_frame(&state);
        state.prev_rx = rx;
        state.prev_tx = tx;
    }

    show_cursor();
    putchar('\n');
    fflush(stdout);
    return 0;
}
