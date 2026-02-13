#include <QApplication>
#include <QFile>
#include <QIcon>
#include "windows/SearchAppWindow.h"
#include "windows/SystemTray.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("SearchTool_Standalone");
    a.setOrganizationName("SearchTool_Standalone");
    a.setWindowIcon(QIcon(":/icons/app_icon.ico"));
    
    // 设置退出策略：最后一个窗口关闭时不退出程序，由托盘控制退出
    a.setQuitOnLastWindowClosed(false);

    // 加载全局样式表
    QFile styleFile(":/qss/dark_style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        a.setStyleSheet(styleFile.readAll());
    }

    SearchAppWindow* w = new SearchAppWindow();
    
    // 初始化托盘
    SystemTray* tray = new SystemTray(&a);
    QObject::connect(tray, &SystemTray::showWindow, [w](){
        if (w->isVisible()) {
            w->hide();
        } else {
            w->show();
            w->raise();
            w->activateWindow();
        }
    });
    QObject::connect(tray, &SystemTray::quitApp, &a, &QApplication::quit);
    
    tray->show();
    w->show();

    return a.exec();
}