# zxnetmon

Nyx 风格的实时网络流量面板。C 实现，只读 `/proc/net/dev`，不依赖 ncurses、awk 或外部库。

源码：

- `main.c`：命令行参数
- `netdev.c`：读取 `/proc/net/dev` 计数器、列出接口
- `render.c`：ANSI 画面和 30 点 ASCII 图
- `ui.c`：采样循环、SIGINT/SIGTERM 清理、光标恢复
- `zxnetmon.h`

## 构建

```bash
./configure --host=native --tools=zxnetmon
make zxnetmon
timeout 2 ./bin/host/zxnetmon -i lo -n 1
```

交叉构建使用默认 `configure --host=arm-linux-musleabi`，产物为 `bin/zxnetmon`。

## 用法

```bash
zxnetmon
zxnetmon -i br0
zxnetmon -i br0 -n 1
zxnetmon -h
```

- `-i / --interface`：接口名，默认 `br0`
- `-n / --interval`：刷新间隔秒数，默认 `1`，范围 1–86400
- `Ctrl+C` 退出并恢复光标

## 显示

```text
========================================
          ZX NETMONITOR v1.2.0
========================================

Interface: br0

Download:   2.45 MB/s
Upload:     128.50 KB/s

RX Total:   1.28 GB
TX Total:   256.40 MB

----------------------------------------
        REALTIME TRAFFIC (last 30 sec)
----------------------------------------
  3.0 MB/s |
  2.0 MB/s |       *
  1.0 MB/s |   *   *       *
  0.0 B/s  +------------------------------
Legend: * RX   + TX   # both
Press Ctrl+C to exit
```

数据来自 `/proc/net/dev`；速率是相邻采样的差值，只保留最近 30 个采样点。计数器回绕或接口重置时速率按 0 处理。

## 注意

- 这是实时终端画面，最好在 Telnet 里运行；`ssh -T` 没有 PTY，可以用 `timeout 5 zxnetmon` 做一次性观察。
- 不会写文件，不会抓包，不会改网络配置。
