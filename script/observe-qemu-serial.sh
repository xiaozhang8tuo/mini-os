#!/bin/bash

# 创建 /tmp/qemu-serial 文件
touch /tmp/qemu-serial

# 清空文件内容
> /tmp/qemu-serial

# 使用 tail 命令实时观察文件输出
tail -f /tmp/qemu-serial