# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -Wextra -O2 -I./src -g

# 源文件目录
SRC_DIR = src

# 目标文件目录
BUILD_DIR = build

# 最终可执行文件
TARGET = $(BUILD_DIR)/BUPTscsOS

# 获取所有 .c 文件
SRCS = $(wildcard $(SRC_DIR)/*.c)

# 将 .c 文件转换为 .o 文件，并放在 build 目录下
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 编译 .c 文件为 .o 文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR)

# 伪目标
.PHONY: all clean