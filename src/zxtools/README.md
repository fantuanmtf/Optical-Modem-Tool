# zxtools

工具箱入口，纯 POSIX `sh`。

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

`zxtools` 会优先调用同目录下的工具，再回退到 `PATH`；所以直接运行源码目录里的脚本、或安装后的 `/usr/bin/zxtools` 都可以。

## 注意

- 菜单需要 TTY。`ssh -T` 没有 TTY 时会打印用法，请使用 `zxtools <子命令>`。
- 所有子工具都是只读的；不会改配置。
