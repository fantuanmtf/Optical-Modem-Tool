# zxtools

工具箱入口。C 实现，源码：

- `main.c`：子命令分发、自身目录定位、用法
- `menu.c`：TTY 交互菜单
- `run.c`：`fork`/`execv` 同目录工具，失败后回退 `execvp` 查找 `PATH`
- `zxtools.h`

## 构建

```bash
./configure --host=native --tools=all
make zxtools
./bin/host/zxtools help
```

交叉构建使用默认 `configure --host=arm-linux-musleabi`，产物为 `bin/zxtools`。

## 用法

```bash
zxtools                 # TTY 下打开菜单
zxtools router-check    # 直接运行自检
zxtools fileinfo /usr/data/hello
zxtools netmon
zxtools help
```

菜单项：

1. `router-check` 自检
2. `fileinfo` 文件信息（会提示输入路径）
3. `zxnetmon` 实时流量
4. `router-check --save`
5. 退出

## 行为

- 先执行 `zxtools` 所在目录下的同名二进制，再回退到 `PATH`；所以直接运行 `bin/host/zxtools` 或安装后的 `/usr/bin/zxtools` 都可以。
- 所有子工具都是只读的；不会改配置。
- 菜单需要 TTY。`ssh -T` 没有 TTY 时会打印用法，请使用 `zxtools <子命令>`。
- 没有 shell 包装层：`zxtools` 本身就是 C 二进制，直接 `exec` 子工具。
