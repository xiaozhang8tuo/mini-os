
project(boot LANGUAGES C)

# 使用自定义的链接器
# 这段代码是用于CMake构建系统的，它设置了链接器（linker）的标志（flags），用于生成可执行文件。下面是这些标志的解释：
# -m elf_i386: 这个标志告诉链接器使用 elf_i386 格式来生成可执行文件，这是适用于Intel x86架构的ELF格式。
# -Ttext=0x7c00: 这个标志用于指定代码的加载地址。在这个例子中，它将可执行文件的代码段加载到内存地址 0x7c00 处，这是x86的引导扇区（boot sector）加载代码的标准位置。
# --section-start boot_end=0x7dfe: 这个标志用于定义一个名为 boot_end 的节（section）的加载地址。这个节通常包含了引导扇区的结束位置。在这里，它被加载到内存地址 0x7dfe 处，这样引导扇区的代码和数据就会在加载到内存后保留在一个连续的内存区域中。
# 这些链接器标志通常在需要生成特定类型的可执行文件，如引导扇区（boot sector）或者嵌入式系统中的内核镜像时使用。这里的例子看起来像是在创建一个引导扇区，这是计算机启动过程中的第一个可执行代码，通常用于加载操作系统或者其他引导阶段。
set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386  -Ttext=0x7c00  --section-start boot_end=0x7dfe")

set(CMAKE_C_LINK_EXECUTABLE "${LINKER_TOOL} <OBJECTS> ${CMAKE_EXE_LINKER_FLAGS} -o ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf")

# 将所有的汇编、C文件加入工程
# 注意保证start.asm在最前头，这样对应的程序会放在bin文件开头，从而在加载到0x7c00时能在开头处
file(GLOB C_LIST "*.c" "*.h")
add_executable(${PROJECT_NAME} start.S ${C_LIST})

# bin文件生成，写入到image目录下
add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${OBJCOPY_TOOL} -O binary ${PROJECT_NAME}.elf ${CMAKE_SOURCE_DIR}/image/${PROJECT_NAME}.bin
                   COMMAND ${OBJDUMP_TOOL} -x -d -S -m i8086 ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_dis.txt
                   COMMAND ${READELF_TOOL} -a ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_elf.txt
)