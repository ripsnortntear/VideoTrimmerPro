#include <QApplication>
#include <QIcon>
#include <QDir>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Video Trimmer Pro");
    app.setOrganizationName("MediaTools");

    // Attempt to load runtime icon from app dir if present, or fallback to compiled resource
    QString iconPath = QCoreApplication::applicationDirPath() + "/app_icon.ico";
    if (QFile::exists(iconPath)) {
        app.setWindowIcon(QIcon(iconPath));
    }

    MainWindow window;
    window.show();

    return app.exec();
}