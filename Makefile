CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lsqlite3 -lcrypto

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = $(BIN_DIR)/tests

# 源文件和目标文件
SRCS = $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(SRC_DIR)/crypto/*.c) \
       $(wildcard $(SRC_DIR)/db/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# 主程序
MAIN_SRCS = $(SRC_DIR)/main.c \
            $(SRC_DIR)/crypto/ecn_crypto.c \
            $(SRC_DIR)/db/ecn_db.c
MAIN_OBJS = $(MAIN_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/ecn

# 加密模块测试程序
CRYPTO_TEST_SRCS = $(SRC_DIR)/crypto/ecn_crypto_test.c \
                   $(SRC_DIR)/crypto/ecn_crypto.c
CRYPTO_TEST_OBJS = $(CRYPTO_TEST_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CRYPTO_TEST_TARGET = $(TEST_DIR)/crypto_test

# 数据库模块测试程序
DB_TEST_SRCS = $(SRC_DIR)/db/ecn_db_test.c \
               $(SRC_DIR)/db/ecn_db.c
DB_TEST_OBJS = $(DB_TEST_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DB_TEST_TARGET = $(TEST_DIR)/db_test

# 默认目标
all: directories $(TARGET) $(CRYPTO_TEST_TARGET) $(DB_TEST_TARGET)

# 创建必要的目录
directories:
	@mkdir -p $(OBJ_DIR)/crypto
	@mkdir -p $(OBJ_DIR)/db
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(TEST_DIR)

# 编译规则
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 链接主程序
$(TARGET): $(MAIN_OBJS)
	$(CC) $(MAIN_OBJS) -o $(TARGET) $(LDFLAGS)

# 链接加密测试程序
$(CRYPTO_TEST_TARGET): $(CRYPTO_TEST_OBJS)
	$(CC) $(CRYPTO_TEST_OBJS) -o $(CRYPTO_TEST_TARGET) $(LDFLAGS)

# 链接数据库测试程序
$(DB_TEST_TARGET): $(DB_TEST_OBJS)
	$(CC) $(DB_TEST_OBJS) -o $(DB_TEST_TARGET) $(LDFLAGS)

# 运行所有测试
test: $(CRYPTO_TEST_TARGET) $(DB_TEST_TARGET)
	./$(CRYPTO_TEST_TARGET)
	./$(DB_TEST_TARGET)

# 运行加密测试
test-crypto: $(CRYPTO_TEST_TARGET)
	./$(CRYPTO_TEST_TARGET)

# 运行数据库测试
test-db: $(DB_TEST_TARGET)
	./$(DB_TEST_TARGET)

# 清理规则
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -f test.db

# 安装规则
install: all
	install -m 755 $(TARGET) /usr/local/bin/

.PHONY: all clean install directories test test-crypto test-db
