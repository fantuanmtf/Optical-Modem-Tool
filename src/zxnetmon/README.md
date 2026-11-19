# zxnetmon

Nyx 风格的实时网络流量面板，纯 POSIX `sh` + `awk`，不依赖 ncurses。

## 用法

```bash
zxnetmon
zxnetmon -i br0
zxnetmon -i br0 -n 1
zxnetmon -h
```

- `-i / --interface`：接口名，默认 `br0`
- `-n / --interval`：刷新间隔秒数，默认 `1`
- `Ctrl+C` 退出并恢复光标

## 显示

```text
========================================
          ZX NETMONITOR v1.0
========================================

Interface: br0

Download:       2.45 MB/s
Upload:         128.50 KB/s

RX Total:       1.28 GB
TX Total:       256.40 MB

----------------------------------------
        REALTIME TRAFFIC
----------------------------------------
  3.0 MB/s |
  2.0 MB/s |       *
  1.0 MB/s |   *   *       *
  0.0 MB/s +--------------------------
              1   2   3   4   5 sec

  * RX   + TX   # RX+TX

========================================
Press Ctrl+C to exit
```

数据来自 `/proc/net/dev`；速率是相邻采样的差值。只保留最近 30 个采样点。

## 注意

- 这是实时终端画面，最好在 Telnet 里运行；`ssh -T` 没有 PTY，可以用 `timeout 5 zxnetmon` 做一次性观察。
- 不会写文件，不会抓包，不会改网络配置。
