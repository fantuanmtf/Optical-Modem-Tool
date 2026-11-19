# router-check

光猫自检报告。C 实现，数据优先来自 `/proc`、`/sys` 和 `statvfs(2)`，不依赖 netstat/route/ps/ifconfig。

源码：

- `main.c`：参数、输出文件、报告顺序
- `system.c`：系统信息、CPU、进程
- `memory.c`：`/proc/meminfo`
- `storage.c`：`/proc/mounts` + `statvfs(2)`，回退 `df`
- `netinfo.c`：网卡、路由、监听端口、bridge、ARP、无线
- `report.c`：stdout + 文件 tee、可选命令输出
- `router-check.h`

## 构建

```bash
./configure --host=native --tools=router-check
make router-check
./bin/host/router-check -o /tmp/router-check.txt
```

交叉构建使用默认 `configure --host=arm-linux-musleabi`，产物为 `bin/router-check`。

## 用法

```bash
router-check
router-check --save
router-check -o /tmp/router-check.txt
router-check -h
```

- 默认打印到终端。
- `--save` 保存到 `${ZXTOOLS_HOME:-/usr/data/zxtools}/reports/router-check-<时间>.txt`，同时仍打印。
- `-o FILE` 保存到指定文件，同时仍打印。

## 报告内容

- System：日期、hostname、`uname(2)`、`/proc/uptime`、loadavg、`/etc/openwrt_release`
- CPU：`/proc/cpuinfo` 的 processor/model/Hardware/Revision/BogoMIPS 字段
- Memory：`/proc/meminfo` 和粗略使用率
- Storage：`/proc/mounts` + `statvfs` 的容量/使用率；不可用时回退 `df -h`/`df -k`
- Network：`getifaddrs(3)` 地址/掩码/MAC，附 `/proc/net/dev` 计数
- Routes：解析 `/proc/net/route`
- Listening：解析 `/proc/net/tcp`、`tcp6`、`udp`、`udp6`
- Bridge：`/sys/class/net/*/bridge`、`brif`
- ARP：`/proc/net/arp`
- Processes：`/proc/<pid>/status`、`cmdline`，按 PID 排序
- Wireless：`/proc/net/wireless`，不可用时尝试 `iwconfig`

在 BusyBox 环境里缺失的数据源会自动标注或跳过，不会让整份报告失败。报告只读，不修改系统配置。
