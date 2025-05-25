cmake_minimum_required(VERSION 3.10)

project(ECN C CXX)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置C++标准（用于Qt客户端）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置Qt自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找所需的包
find_package(SQLite3 REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# 添加GmSSL库
find_library(GMSSL_LIBRARY
    NAMES gmssl
    PATHS /usr/local/lib /usr/lib
)
if(NOT GMSSL_LIBRARY)
    message(FATAL_ERROR "GmSSL library not found")
endif()

# 添加头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/usr/local/include) # GmSSL headers
include_directories(${SQLite3_INCLUDE_DIRS})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# 设置源文件
set(COMMON_SOURCES
    src/crypto/ecn_crypto.c
    src/db/ecn_db.c
    src/server/ecn_server.c
)

set(SERVER_SOURCES
    src/main.c
    ${COMMON_SOURCES}
)

set(TEST_SOURCES
    src/test/test_main.c
    src/test/test_crypto.c
    src/test/test_server.c
    ${COMMON_SOURCES}
)

set(CLIENT_SOURCES
    src/client/main.cpp
    src/client/mainwindow.cpp
    src/client/logindialog.cpp
    src/client/notelistwidget.cpp
    src/client/noteeditwidget.cpp
)

# 创建可执行文件
add_executable(ecn_server ${SERVER_SOURCES})
add_executable(ecn_test ${TEST_SOURCES})
add_executable(ecn_client ${CLIENT_SOURCES})

# 链接库
target_link_libraries(ecn_server
    ${GMSSL_LIBRARY}
    ${SQLite3_LIBRARIES}
    pthread
)

target_link_libraries(ecn_test
    ${GMSSL_LIBRARY}
    ${SQLite3_LIBRARIES}
    pthread
)

target_link_libraries(ecn_client
    Qt5::Core
    Qt5::Widgets
    ${GMSSL_LIBRARY}
)

# 设置输出目录
set_target_properties(ecn_server ecn_test ecn_client
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 添加测试
enable_testing()
add_test(NAME unit_tests COMMAND ecn_test) 