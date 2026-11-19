# Optical-Modem-Tool

给已取得 root 的 ARM 光猫用的 **纯 Shell 小工具集**，面向 BusyBox / ZXIC Linux 4.1.25+ 这类环境。没有 C 代码、没有预编译二进制、不依赖包管理器。

> 仅用于你自己的设备，或已获得明确授权的设备。root 权限意味着可以改坏系统，请先备份。

## 先取得 Root

这个仓库**不负责提权**。请先用另一个 fork 取得光猫 root / SSH：

- 仓库：<https://github.com/fantuanmtf/ung220z-tool>
- 运行其中的 `ung220z.py`，它会提取 Web、Telnet、PPPoE、WiFi、root 等凭据，并可安装仅密钥、仅 LAN 的 SSH。
- 该固件的 SSH **PTY 交互终端不兼容**；命令请用 `ssh -T`，完整终端用 Telnet（登录后 `su`）。
- 拿到 root 后，把本仓库传到光猫，例如：

```bash
scp -r Optical-Modem-Tool root@192.168.1.1:/usr/data/
ssh -T root@192.168.1.1 'cd /usr/data/Optical-Modem-Tool && sh install.sh'
```

## 项目

| 目录 | 命令 | 作用 |
| --- | --- | --- |
| `zxtools/` | `zxtools` | 工具箱菜单，也可直接调用子命令 |
| `router-check/` | `router-check` | 系统、内存、存储、网络、进程自检报告 |
| `fileinfo/` | `fileinfo` | 文件类型、权限、大小、owner/group、mtime |
| `zxnetmon/` | `zxnetmon` | Nyx 风格的实时 RX/TX 流量面板 |

全部是 POSIX `sh`，可在 BusyBox ash 下运行。

## 安装

在光猫 root shell 中执行：

```bash
cd /usr/data/Optical-Modem-Tool
sh install.sh
```

默认安装到 `/usr/data/zxtools`，并在 `/usr/bin` 下建立 `zxtools`、`router-check`、`fileinfo`、`zxnetmon` 链接。若 `/usr/bin` 不可写，则链接放到 `/usr/data/zxtools/bin`。

重启后 `/usr/bin` 的链接可能丢失。要持久化：

```bash
sh install.sh --boot
```

`--boot` 会在现有 `/usr/data/burntest/burntest.sh` 末尾追加一个带 `# ZXTOOLS_BOOT` 标记的启动块，并先备份为 `burntest.sh.zxtools.bak`。不会覆盖 SSH 的启动内容。

## 使用

```bash
zxtools                 # 菜单
zxtools router-check    # 直接调用
router-check            # 打印自检报告
router-check --save     # 保存到 /usr/data/zxtools/reports/
router-check -o /tmp/router-check.txt
fileinfo /usr/data/hello
zxnetmon                # 默认 br0，1 秒刷新
zxnetmon -i br0 -n 1
```

注意：

- `zxtools` 菜单需要 TTY；通过 `ssh -T` 没有 TTY 时会打印用法，请直接用子命令。
- `zxnetmon` 需要终端看实时画面；Telnet 最稳。SSH 下可以用 `ssh -T ... 'timeout 5 zxnetmon'` 做一次性观察。
- 所有工具默认只读，不监听端口，不改防火墙。

## 兼容性与限制

- 目标：ARMv7 光猫，BusyBox，`/proc/net/dev`、`/proc/meminfo`、`ifconfig`/`ip`、`netstat`、`df`、`ps` 等常见命令。
- 没有历史数据库；`zxnetmon` 只保留最近 30 个采样点。
- 设备内存/存储有限，不建议在光猫上跑重负载服务。
- 固件升级或恢复出厂可能清空 `/usr/data`，工具和启动块都需要重装。

## 卸载

```bash
cd /usr/data/Optical-Modem-Tool
sh uninstall.sh
```

它会删除本工具创建的链接和 `/usr/data/zxtools`，并尝试移除启动块（保留 `.zxtools.uninstall.bak` 备份）。

## 安全

- 这是 root 级工具；不要放在公网，不要把 SSH/Telnet 端口映射到 WAN。
- 不要把我给你的示例凭据写进仓库或脚本。
- 建议只在可信 LAN 使用，并保留光猫原厂配置备份。

## License

见 [LICENSE](LICENSE)。
