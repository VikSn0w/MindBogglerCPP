#include <iostream>

#include "MainWindow/MainWindow.h"
#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtGui/QIcon>
#include <QtCore/QFileInfo>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("MindBoggler++");
    app.setApplicationDisplayName("MindBoggler++ - Brainfuck IDE");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("VittoioPicone");
    app.setOrganizationDomain("mindbogglerapp.com");

    QIcon appIcon;

    QString executableDir = QApplication::applicationDirPath();

    QStringList iconPaths = {
        executableDir + "/resources/icon.ico",    
        executableDir + "/resources/icon.png",    
        "resources/icon.ico",                     
        "./resources/icon.ico",                   
        QDir::currentPath() + "/resources/icon.ico", 
    };

    std::cout << "=== LOADING ICON ===" << std::endl;
    for (const QString& path : iconPaths) {
        if (QFile::exists(path)) {
            appIcon = QIcon(path);
            std::cout << "Using icon: " << path.toStdString() << std::endl;
            break;
        }
    }

    if (!appIcon.isNull()) {
       
        app.setWindowIcon(appIcon);
        std::cout << "Application icon set successfully" << std::endl;
    } else {
        std::cout << "No icon found" << std::endl;
    }

    MainWindow window;

   
    if (!appIcon.isNull()) {
        window.setWindowIcon(appIcon);
        std::cout << "MainWindow icon set successfully" << std::endl;
    }

    window.show();

    return app.exec();
}