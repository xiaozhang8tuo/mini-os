# 调试过程 #

通过指令`qemu` ` view` `compatmonitor0` : `info registers` 查看进入保护模式后各个**段选择子**的状态，以及**GDT**

![image-20230320001606641](.assets/image-20230320001606641.png)

`info mem` 查看内存地址的映射

![image-20230802164601950](.assets/image-20230802164601950.png)



线性地址0x80000000 

![image-20230802172745956](.assets/image-20230802172745956.png)

# 问题解决记录 #

1 debug时配置的类型cppdbg不受支持，重启vscodec/c++扩展

