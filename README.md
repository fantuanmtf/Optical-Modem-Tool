# Optical-Modem-Tool

面向已 root 的 ARM Linux 光猫 / 路由器的**只读诊断工具集**。工具已从 shell 重写为可读的 C 源码，使用 POSIX/Linux API 和 `/proc`、`/sys`，不依赖 ncurses 或运行时包管理器。

源码全部在 `src/`；编译产物进入 git 忽略的 `bin/`，本地配置进入 `config.mk`。旧仓库 [`ung220z-tool`](https://github.com/fantuanmtf/ung220z-tool) 只负责提权和获取 root/SSH，不包含这些工具。

> 仅用于你自己的设备或已授权的设备。不要把这些工具、SSH 或管理端口暴露到公网。

## 先获取 Root

本仓库不包含提权代码。先在旧仓库获取 root 和 SSH：

- 仓库：<https://github.com/fantuanmtf/ung220z-tool>
- 运行 `ung220z.py`：提取 Web/Telnet/PPPoE/WiFi/root 凭据，并安装仅密钥的 LAN SSH。
- 该固件的交互式 SSH PTY 不完整：SSH 用 `ssh -T`，完整终端用 Telnet。
- 拿到 root 后，再回到本仓库编译/安装工具。

## 项目

| 项目 | 说明 |
|---|---|
| `zxtools` | 统一菜单入口，也可直接调用子工具 |
| `router-check` | 系统、CPU、内存、存储、网卡、路由、端口、进程、无线信息自检 |
| `fileinfo` | 文件类型、权限、大小、owner/group、mtime、symlink 目标 |
| `zxnetmon` | `ZX NETMONITOR` 实时 RX/TX 速率、累计流量和 30 点 ASCII 图 |

每个项目的源码和说明在 `src/<项目>/`。共享代码在 `src/common/` 和 `src/include/`。

## 编译

### 依赖

- 编译机：Linux、`make`、POSIX `sh`。
- 目标机：ARMv7 softfloat、Linux 4.1.25、BusyBox；无需运行编译器。
- 交叉工具链：仓库默认按 `arm-linux-musleabi-gcc` 查找，也可以在 `--with-toolchain` 或 `CC` 中指定。
- 主机测试：x86_64 上的 `cc`。

### 配置与构建

```bash
./configure --host=arm-linux-musleabi
make                # 或 make arm，交叉编译到 bin/
make check          # 用主机 cc 编译到 bin/host/ 并运行 tests/smoke.sh
```

如果没有把交叉工具链加入 `PATH`：

```bash
./configure --host=arm-linux-musleabi \
    --with-toolchain "$HOME/.local/opt/arm-linux-musleabi-cross/bin"
```

只构建部分工具：

```bash
./configure --tools="fileinfo zxnetmon"
make
```

主机本地构建（测试/桌面 Linux）：

```bash
./configure --host=native --tools=all
make
./bin/host/zxtools help
```

`configure` 是非交互的 POSIX `sh` 脚本，支持：

```text
--host=TARGET         目标（默认 arm-linux-musleabi；native 为主机）
--prefix=DIR          安装目录（默认 /usr/data/zxtools）
--bindir=DIR          命令链接目录（默认 /usr/bin）
--tools=LIST          空格/逗号分隔的工具列表，或 all
--with-toolchain=DIR  额外搜索编译器的目录
-h, --help
```

环境变量 `CC`、`CFLAGS`、`CPPFLAGS`、`LDFLAGS`、`HOSTCC` 会被采用。ARM 构建始终追加：

```text
-Os -march=armv7-a -mfloat-abi=soft -fno-pie -static -no-pie
```

可以用下面的命令检查产物：

```bash
file bin/fileinfo bin/zxnetmon bin/router-check bin/zxtools
readelf -A bin/fileinfo | grep -i vfp   # 应无输出
```

`bin/` 和 `config.mk` 已被 `.gitignore` 忽略；`make clean` 删除 `bin/`，`make distclean` 同时删除 `config.mk`。

## 安装

### 光猫 / 已 root 的 BusyBox 设备

先在编译机交叉构建并打包，再把源码包和 `bin/` 中的 ARM 二进制带到设备；或直接在设备上编译（不推荐，设备通常没有编译器）。

```bash
make
sh install.sh --modem
```

默认安装到 `/usr/data/zxtools`，并在 `/usr/bin` 创建所选工具的链接。

可选开机重建链接：

```bash
sh install.sh --modem --boot
```

`--boot` 会往 `/usr/data/burntest/burntest.sh` 追加一段 `# ZXTOOLS_BOOT`，并先备份为 `.zxtools.bak`。该钩子原本由 `ung220z-tool` 创建；如果你卸载 SSH 时删除了它，需要重新安装或手动恢复。

只安装部分工具：

```bash
sh install.sh --modem --tools "fileinfo zxnetmon"
```

不创建链接：

```bash
sh install.sh --modem --no-links
```

`install.sh` 默认从 `<源码>/bin/` 复制二进制；使用主机构建或自定义目录时：

```bash
sh install.sh --local --from bin/host
```

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
make all                           # 构建所选工具
make check                         # 主机编译 + tests/smoke.sh
make package                       # 生成 bin/optical-modem-tool-<version>.tar.gz（纯源码）
make install PREFIX=/usr/data/zxtools BINDIR=/usr/bin
make uninstall
make clean                         # 删除 bin/
```

`make package` 只打包源码和安装脚本，不包含编译产物；目标机安装前需要先在编译机构建并把 `bin/` 一起复制过去。

## 使用

```bash
zxtools                 # 交互菜单（需要 TTY，建议 Telnet）
zxtools router-check    # 非交互直调
zxtools fileinfo /usr/data/hello
zxtools netmon

router-check
router-check --save     # 保存到 $ZXTOOLS_HOME/reports/ 或 /usr/data/zxtools/reports/
router-check -o /tmp/report.txt
fileinfo /usr/data/hello
zxnetmon -i br0 -n 1
```

- `zxtools` 菜单需要 TTY；通过 `ssh -T` 运行时请用 `zxtools router-check` 这种直调方式。非 TTY 会自动打印用法。
- `zxnetmon` 需要终端显示 ANSI 图；建议用 Telnet，`Ctrl+C` 退出并恢复光标。
- `router-check` 和 `fileinfo` 可以在 `ssh -T` 下直接运行。
- 所有工具只读：不抓包、不改网络配置、不写系统文件（`--save`/`-o` 除外）。

## 目录结构

```text
Optical-Modem-Tool/
├── configure                 # 手写 POSIX configure
├── Makefile                  # 含 config.mk；all/arm/check/package/install
├── install.sh                # 场景安装：modem/local/custom
├── uninstall.sh
├── boot-links.sh
├── src/
│   ├── include/zxcommon.h    # 共享声明
│   ├── include/zxcompat.h    # 非标准 libc 兼容层
│   ├── common/util.c
│   ├── common/compat.c
│   ├── zxtools/              # main.c menu.c run.c
│   ├── router-check/         # main.c system.c memory.c storage.c netinfo.c report.c
│   ├── fileinfo/             # main.c inspect.c
│   └── zxnetmon/             # main.c netdev.c render.c ui.c
├── tests/smoke.sh            # 主机原生冒烟测试
├── bin/                      # 编译产物，git 忽略
├── config.mk                 # configure 生成，git 忽略
└── README.md
```

## 限制

- 目标为 ARMv7 softfloat 静态二进制；核心功能不依赖设备上的 shell、awk 或外部库。只有 `df`/`iwconfig` 这类可选回退会调用 BusyBox 命令。
- `zxnetmon` 只做 1 秒采样（可调），轮询 `/proc/net/dev`；不抓包。
- `router-check` 的存储信息优先用 `statvfs(2)`，不可用时才回退 `df`；无线优先 `/proc/net/wireless`，不可用时才尝试 `iwconfig`。
- 光猫内存/存储有限；进程列表和报告会较长，建议 `-o` 保存后查看。
- `/usr/data` 是持久分区，但固件升级/恢复出厂可能清空；升级后重新安装。
- 交互式 SSH PTY 在这类 ZTE 固件上不可靠；完整终端用 Telnet。
- 只读诊断优先；改配置前先备份。

## 卸载

```bash
sh uninstall.sh --modem
sh uninstall.sh --local
sh uninstall.sh --prefix /opt/zxtools --bin-dir /usr/local/bin
```

默认会移除本工具创建的链接、安装目录和 `# ZXTOOLS_BOOT` 启动块；保留启动块可加 `--keep-boot`。只移除部分工具：

```bash
sh uninstall.sh --modem --tools "fileinfo zxnetmon"
```

部分卸载会保留启动块，避免影响仍安装的工具。

## License

见 [LICENSE](LICENSE)。
