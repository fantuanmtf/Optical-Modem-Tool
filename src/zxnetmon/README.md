# zxnetmon

Nyx 风格的实时网络流量面板。C 实现，只读 `/proc/net/dev`，不依赖 ncurses、awk 或外部库。

源码：

- `main.c`：命令行参数、自动检测模式
- `netdev.c`：读取 `/proc/net/dev` 计数器、枚举/筛选接口
- `render.c`：单接口画面、多接口紧凑面板、30 点 ASCII 图
- `ui.c`：采样循环、SIGINT/SIGTERM 清理、光标恢复
- `zxnetmon.h`

## 构建

```bash
./configure --host=native --tools=zxnetmon
make check
timeout 2 ./bin/host/zxnetmon -n 1
```

交叉构建使用默认 `configure --host=arm-linux-musleabi`，产物为 `bin/zxnetmon`。

## 用法

```bash
zxnetmon                 # 自动检测虚拟接口，每个接口一个面板
zxnetmon --list          # 只打印自动检测结果，不监控
zxnetmon -n 1            # 自动检测 + 1 秒采样
zxnetmon -i br0          # 只监控指定接口
zxnetmon -i br0 -n 1
zxnetmon -i all          # 所有非 lo 接口
zxnetmon -h
```

- 不写 `-i` 时只按名称白名单自动检测：
  - 永远排除 `lo`，并显式跳过 `gre*`、`gretap*`、`sit*`、`ip6tnl*`、`idm*`。
  - 只选择名称匹配 `br*`、`vlan*`、`ppp*`、`wlan*`、`wlp*`、`tun*`、`tap*`、
    `bond*`、`docker*`、`virbr*`、`vmnet*` 的接口。
  - 不再使用 `/sys/class/net/<if>/device` 作为判断条件。
  - 没有匹配到白名单接口时，回退到所有非 `lo` 接口（仍跳过上面的黑名单）。
  - 没有任何可用接口时打印错误并退出。
- `-i / --interface`：显式指定时保持单接口模式；`all` 表示所有非 `lo` 接口。
- `-n / --interval`：刷新间隔秒数，默认 `1`，范围 1–86400。
- `-l / --list / --dry-run`：只打印选中的接口名并退出，不进入监控；可配合
  `-i IFACE` 或 `-i all` 使用。
- `Ctrl+C` 退出并恢复光标。

在这类光猫上，自动模式通常只选中 `br0` 和 `ppp0`；在桌面 Linux 上通常是
`wlan0` 或 `wlp*`。可以用下面的命令核对：

```bash
zxnetmon --list
```

```text
br0
ppp0
```

## 显示

单接口模式（`-i br0`）：

```text
========================================
          ZX NETMONITOR v1.3.0
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

自动检测模式会打印一次标题，然后为每个接口输出一个紧凑面板：

```text
========================================
          ZX NETMONITOR v1.3.0
========================================
Interfaces: 2 (auto-detected)

----------------------------------------
Interface: br0
Download:   2.45 MB/s
Upload:     128.50 KB/s
RX Total:   1.28 GB
TX Total:   256.40 MB
REALTIME TRAFFIC (last 30 sec)
----------------------------------------
  3.0 MB/s |       *
  2.0 MB/s |   *   *
  1.0 MB/s |   *       *
  0.0 B/s  +------------------------------
...
```

数据来自 `/proc/net/dev`；速率是相邻采样的差值。每个接口独立保留最近 30 个采样点。计数器回绕或接口重置时速率按 0 处理。`*` 表示 RX，`+` 表示 TX，`#` 表示两者。

## 注意

- 这是实时终端画面，最好在 Telnet 里运行；`ssh -T` 没有 PTY，可以用 `timeout 5 zxnetmon` 做一次性观察。
- 接口很多时自动检测会把所有匹配的虚拟接口都画出来，必要时用 `-i IFACE` 限定。
- 不会写文件，不会抓包，不会改网络配置。
