CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -I./include
CXXFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lsqlite3 -lgmssl

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = $(BIN_DIR)/tests

# 源文件
SERVER_SRCS = $(SRC_DIR)/server/main.c \
              $(SRC_DIR)/server/ecn_server.c \
              $(SRC_DIR)/crypto/ecn_crypto.c \
              $(SRC_DIR)/db/ecn_db.c

CRYPTO_TEST_SRCS = $(SRC_DIR)/crypto/ecn_crypto_test.c \
                   $(SRC_DIR)/crypto/ecn_crypto.c

DB_TEST_SRCS = $(SRC_DIR)/db/ecn_db_test.c \
               $(SRC_DIR)/db/ecn_db.c \
               $(SRC_DIR)/crypto/ecn_crypto.c

# 目标文件
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CRYPTO_TEST_OBJS = $(CRYPTO_TEST_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DB_TEST_OBJS = $(DB_TEST_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# 可执行文件
SERVER_TARGET = $(BIN_DIR)/ecn_server
CRYPTO_TEST_TARGET = $(TEST_DIR)/crypto_test
DB_TEST_TARGET = $(TEST_DIR)/db_test

# GUI 目标
GUI_TARGET = $(BIN_DIR)/ecn-gui

.PHONY: all clean gui test

all: directories $(SERVER_TARGET) $(CRYPTO_TEST_TARGET) $(DB_TEST_TARGET) gui

# 创建目录
directories:
	@mkdir -p $(OBJ_DIR)/crypto
	@mkdir -p $(OBJ_DIR)/db
	@mkdir -p $(OBJ_DIR)/server
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(TEST_DIR)

# 编译规则
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# 链接规则
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(CRYPTO_TEST_TARGET): $(CRYPTO_TEST_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(DB_TEST_TARGET): $(DB_TEST_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

# GUI 构建规则
gui: directories
	@echo "Building GUI..."
	cd src/gui && qmake ECN.pro -o Makefile && make
	@if [ -f src/gui/ECN ]; then \
		cp src/gui/ECN $(BIN_DIR)/ecn-gui; \
		echo "GUI build successful"; \
	else \
		echo "GUI build failed"; \
		exit 1; \
	fi

# 测试规则
test: $(CRYPTO_TEST_TARGET) $(DB_TEST_TARGET)
	$(CRYPTO_TEST_TARGET)
	$(DB_TEST_TARGET)

# 清理规则
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -f test.db
	cd src/gui && if [ -f Makefile ]; then make clean; rm -f Makefile; fi
