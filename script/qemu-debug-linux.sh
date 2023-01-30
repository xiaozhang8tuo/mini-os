# 适用于Linux
qemu-system-i386 -daemonize -m 128M -s -S  -drive file=disk.img,index=0,media=disk,format=raw

# -daemonize 
#   Daemonize the QEMU process after initialization.  
#   QEMU will not detach from standard IO until it is ready to receive connections on any of its devices.  
#   This option is a useful way for external programs to launch QEMU without having to cope with initialization race conditions.

# -m 
#   set RAM size

# -s 
#   Shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234.

# -S
#   Do not start CPU at startup (you must type 'c' in the monitor).

# -drive
#   format指定将使用哪种磁盘格式，而不是检测格式。可用于指定format=raw以避免解释不受信任的格式标头。
#   media
#   index
#   file