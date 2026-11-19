# Optical-Modem-Tool

面向已 root 的 ARM Linux 光猫 / 路由器的**纯 shell 工具集**。  
源码全部在 `src/`；`bin/` 只用于本地生成发布包，**不进入 git**。旧仓库 [`ung220z-tool`](https://github.com/fantuanmtf/ung220z-tool) 只负责提权和获取 root/SSH，不包含这些工具。

> 仅用于你自己的设备或已授权的设备。不要把这些工具、SSH 或管理端口暴露到公网。

## 先获取 Root

本仓库不包含提权代码。先在旧仓库获取 root 和 SSH：

- 仓库：<https://github.com/fantuanmtf/ung220z-tool>
- 运行 `ung220z.py`：提取 Web/Telnet/PPPoE/WiFi/root 凭据，并安装仅密钥的 LAN SSH。
- 该固件的交互式 SSH PTY 不完整：SSH 用 `ssh -T`，完整终端用 Telnet。
- 拿到 root 后，再回到本仓库安装工具。

## 项目

| 项目 | 说明 |
|---|---|
| `zxtools` | 统一菜单入口，也可直接调用子工具 |
| `router-check` | 系统、CPU、内存、存储、网卡、路由、端口、进程、无线信息自检 |
| `fileinfo` | 文件类型、权限、大小、owner/group、mtime、symlink 目标 |
| `zxnetmon` | `ZX NETMONITOR` 实时 RX/TX 速率、累计流量和 ASCII 图 |

每个项目的源码和说明在 `src/<项目>/`。

## 安装

### 光猫 / 已 root 的 BusyBox 设备

```bash
tar xzf optical-modem-tool-*.tar.gz
cd Optical-Modem-Tool
sh install.sh --modem
```

默认安装到 `/usr/data/zxtools`，并在 `/usr/bin` 创建 `zxtools`、`router-check`、`fileinfo`、`zxnetmon` 链接。

可选开机重建链接：

```bash
sh install.sh --modem --boot
```

`--boot` 会往 `/usr/data/burntest/burntest.sh` 追加一段 `# ZXTOOLS_BOOT`，并先备份为 `.zxtools.bak`。该钩子原本由 `ung220z-tool` 创建；如果你卸载 SSH 时删除了它，需要重新安装或手动恢复。

### 普通 Linux（不需要 root）

```bash
sh install.sh --local
```

安装到 `${XDG_DATA_HOME:-$HOME/.local/share}/zxtools`，链接到 `~/.local/bin`。

### 自定义目录

```bash
sh install.sh --prefix /opt/zxtools --bin-dir /usr/local/bin
sh install.sh --prefix /opt/zxtools --no-links
```

### Makefile

```bash
make check                         # sh -n + tests/smoke.sh
make package                       # bin/optical-modem-tool-<version>.tar.gz
make install PREFIX=/usr/data/zxtools BINDIR=/usr/bin
make uninstall
make clean                         # 删除 bin/ 发布包
```

`bin/` 已被 `.gitignore` 忽略；发布包请单独上传到 release，不要提交进仓库。

## 使用

```bash
zxtools                 # 交互菜单（需要 TTY，建议 Telnet）
zxtools router-check    # 非交互直调
zxtools fileinfo /usr/data/hello
zxtools netmon

router-check
router-check --save     # 保存到 /usr/data/zxtools/reports/
fileinfo /usr/data/hello
zxnetmon -i br0 -n 1
```

- `zxtools` 菜单需要 TTY；通过 `ssh -T` 运行时请用 `zxtools router-check` 这种直调方式。
- `zxnetmon` 需要终端显示 ANSI 图；建议用 Telnet，`Ctrl+C` 退出。
- `router-check` 和 `fileinfo` 可以在 `ssh -T` 下直接运行。

## 目录结构

```text
Optical-Modem-Tool/
├── src/
│   ├── zxtools/zxtools
│   ├── router-check/router-check
│   ├── fileinfo/fileinfo
│   └── zxnetmon/zxnetmon
├── tests/smoke.sh
├── Makefile
├── install.sh
├── uninstall.sh
├── boot-links.sh
├── bin/                 # 发布包，git 忽略
└── README.md
```

## 限制

- 全部为 POSIX shell / BusyBox 兼容脚本，无 C、无预编译二进制。
- 光猫内存/存储有限；`zxnetmon` 只做 1 秒采样，适合轻量监控。
- `/usr/data` 是持久分区，但固件升级/恢复出厂可能清空；升级后重新安装。
- 交互式 SSH PTY 在这类 ZTE 固件上不可靠；完整终端用 Telnet。
- 只读诊断优先；改配置前先备份。

## 卸载

```bash
sh uninstall.sh --modem
sh uninstall.sh --local
sh uninstall.sh --prefix /opt/zxtools --bin-dir /usr/local/bin
```

默认会移除本工具创建的链接、安装目录和 `# ZXTOOLS_BOOT` 启动块；保留启动块可加 `--keep-boot`。

## License

见 [LICENSE](LICENSE)。
