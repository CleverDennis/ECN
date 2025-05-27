# Encrypted Cloud Notes (ECN)

ECN 是一个安全的云笔记应用程序，使用国密算法（SM2/SM3/SM4）进行加密保护。

## 功能特点

- 使用 SM2 进行密钥交换和数字签名
- 使用 SM4-CTR 模式进行笔记内容加密
- 使用 SM3 进行密码哈希和完整性校验
- 支持用户注册和登录
- 支持笔记的创建、读取、更新和删除
- 提供命令行和图形用户界面
- SQLite 数据库存储

## 项目结构

```
ECN/
├── bin/                    # 编译后的可执行文件
├── docs/                   # 文档
├── include/               # 头文件
│   ├── ecn_crypto.h      # 加密模块头文件
│   ├── ecn_db.h          # 数据库模块头文件
│   ├── ecn_note.h        # 笔记模块头文件
│   └── ecn_user.h        # 用户模块头文件
├── lib/                   # 第三方库
├── src/                   # 源代码
│   ├── crypto/           # 加密模块实现
│   ├── db/               # 数据库模块实现
│   ├── gui/              # Qt 图形界面
│   ├── server/           # 服务器实现
│   ├── client/           # 客户端实现
│   ├── test/             # 测试代码
│   └── utils/            # 工具函数
├── obj/                   # 编译中间文件
├── Makefile              # 构建系统
└── CMakeLists.txt        # CMake 构建系统
```

## 依赖项

- GCC/G++ (支持 C++17)
- Qt 5
- SQLite3
- GmSSL (国密算法库)

### Ubuntu/Debian 安装依赖

```bash
# 安装编译工具和基本依赖
sudo apt-get update
sudo apt-get install build-essential cmake

# 安装 Qt 5
sudo apt-get install qt5-default qtcreator

# 安装 SQLite3
sudo apt-get install libsqlite3-dev

# 安装 GmSSL
git clone https://github.com/guanzhi/GmSSL.git
cd GmSSL
./config
make
sudo make install
```

## 编译说明

1. 克隆项目：
```bash
git clone https://github.com/yourusername/ECN.git
cd ECN
```

2. 编译整个项目：
```bash
make clean    # 清理之前的编译
make          # 编译所有组件
```

这将生成以下可执行文件：
- `bin/ecn-gui`：图形界面程序
- `bin/ecn_server`：服务器程序
- `bin/tests/crypto_test`：加密模块测试程序
- `bin/tests/db_test`：数据库模块测试程序

## 运行说明

1. 启动服务器：
```bash
./bin/ecn_server
```

2. 运行图形界面客户端：
```bash
./bin/ecn-gui
```

3. 运行测试：
```bash
make test
```

## 开发说明

### 目录说明

- `src/crypto/`：包含所有加密相关实现
- `src/db/`：数据库操作相关实现
- `src/gui/`：Qt 图形界面实现
- `src/server/`：服务器端实现
- `src/client/`：命令行客户端实现
- `src/test/`：单元测试和集成测试
- `src/utils/`：通用工具函数

### 构建系统

项目提供两种构建系统：
- Makefile：主要构建系统
- CMakeLists.txt：CMake 构建系统（可选）

### GUI 开发

GUI 使用 Qt 5 开发，相关文件在 `src/gui/` 目录下：
- `ECN.pro`：Qt 项目文件
- `mainwindow.ui`：主窗口界面设计
- `logindialog.ui`：登录对话框界面设计

## 许可证

[添加许可证信息]

## 贡献指南

[添加贡献指南]

## 联系方式

[添加联系方式]
