# fileinfo

显示文件/目录/链接的类型、权限、大小、owner/group 和修改时间。C 实现，源码：

- `main.c`：参数处理和帮助
- `inspect.c`：`lstat(2)`、模式字符串、owner/group、symlink 目标、时间格式化
- `fileinfo.h`

## 构建

```bash
./configure --host=native --tools=fileinfo
make fileinfo
./bin/host/fileinfo README.md
```

交叉构建使用默认 `configure --host=arm-linux-musleabi`，产物为 `bin/fileinfo`。

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

- 符号链接使用 `lstat`，输出 `symbolic link -> target`，大小为链接本身。
- 普通文件直接使用 `st_size`；目录标注 `directory entry`；设备标注 `device`。
- `Modified` 只包含时间，不包含文件名。
- 多个路径依次输出，中间空一行；无法读取的路径写 stderr，其他路径继续处理。
