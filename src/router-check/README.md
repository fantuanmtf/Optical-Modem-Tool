# router-check

光猫自检报告，纯 POSIX `sh`。

## 用法

```bash
router-check
router-check --save
router-check -o /tmp/router-check.txt
router-check -h
```

- 默认打印到终端。
- `--save` 保存到 `/usr/data/zxtools/reports/router-check-<时间>.txt`。
- `-o FILE` 保存到指定文件，同时仍然打印。

## 报告内容

- System：日期、hostname、内核、uptime、loadavg
- CPU：`/proc/cpuinfo`
- Memory：`/proc/meminfo` 和粗略使用率
- Storage：`df -h`（不支持时回退 `df -k`）
- Network：`ifconfig -a` 或 `ip addr`、路由、`/proc/net/dev`
- Listening：`netstat -lntp`（回退 `netstat -ln`）
- Bridge / ARP / Processes / Wireless（命令存在时）
- 最后打印报告文件路径（如果保存了）

在 BusyBox 环境里，缺失的命令会自动跳过，不会让整份报告失败。
