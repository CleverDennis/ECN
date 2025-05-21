# ECN (Encrypted Cloud Notes)

## 项目描述
ECN是一个基于国密算法的加密云笔记系统，提供安全的笔记存储和管理功能。系统使用SM2、SM3和SM4等国密算法实现端到端加密，确保用户数据的安全性。

## 主要功能
- 用户认证（SM3加盐哈希）
- 笔记加密存储（SM4-CTR模式）
- 安全通信（SM2加密）
- 审计日志
- 多设备同步支持

## 系统要求
- Linux 5.4+
- GCC 7.3+
- SQLite 3.22+
- OpenSSL 1.1.1+ (with SM2/SM3/SM4 support)
- Qt 5.12+ (可选，用于GUI界面)

## 编译安装
1. 安装依赖：
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential libsqlite3-dev libssl-dev
   
   # CentOS/RHEL
   sudo yum install gcc make sqlite-devel openssl-devel
   ```

2. 克隆项目：
   ```bash
   git clone <repository-url>
   cd ecn
   ```

3. 编译项目：
   ```bash
   make
   ```

4. 安装（可选）：
   ```bash
   sudo make install
   ```

## 目录结构
```
ECN/
├── src/               # 源代码目录
│   ├── main.c        # 主程序入口
│   ├── db/           # 数据库操作
│   ├── gui/          # 图形界面
│   └── utils/        # 工具函数
├── include/          # 头文件
│   ├── ecn_crypto.h  # 加密模块
│   ├── ecn_user.h    # 用户管理
│   ├── ecn_note.h    # 笔记管理
│   ├── ecn_db.h      # 数据库操作
│   └── ecn_audit.h   # 审计日志
├── lib/              # 第三方库
├── docs/             # 文档
├── Makefile          # 编译脚本
└── README.md         # 项目说明
```

## 使用方法
1. 启动程序：
   ```bash
   ecn
   ```

2. 注册新用户：
   ```bash
   ecn register <username>
   ```

3. 登录系统：
   ```bash
   ecn login <username>
   ```

## 开发文档
详细的开发文档请参考 `docs/` 目录。

## 安全说明
- 所有密码使用SM3算法加盐哈希存储
- 笔记内容使用SM4-CTR模式加密
- 通信使用SM2算法加密
- 定期进行安全审计

## 贡献指南
欢迎提交Issue和Pull Request。

## 许可证
本项目采用 MIT 许可证。
