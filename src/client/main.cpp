#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    QApplication::setApplicationName("ECN");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("ECN");
    QApplication::setOrganizationDomain("ecn.org");
    
    // 初始化OpenSSL
    if (OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS | 
                           OPENSSL_INIT_ADD_ALL_DIGESTS, NULL) != 1) {
        QMessageBox::critical(nullptr, "Error",
                            "Failed to initialize OpenSSL");
        return 1;
    }
    
    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
} 