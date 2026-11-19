# fileinfo

显示文件/目录/链接的类型、权限、大小、owner/group 和修改时间，纯 POSIX `sh`。

## 用法

```bash
fileinfo /usr/data/hello
fileinfo /etc /root/.ssh/authorized_keys
```

## 输出示例

```text
Path:     /usr/data/hello
Type:     regular file
Size:     43248 bytes
Mode:     -rwxrwxrwx
Links:    1
Owner:    root
Group:    root
Modified: 2026-11-20 03:31:00
```

- 符号链接会显示 `symlink -> target`。
- 普通文件用 `wc -c` 取字节数；目录显示目录项大小。
- 修改时间优先用 `stat -c '%y'`，BusyBox 不支持时回退到 `ls -ld` 的日期字段。
