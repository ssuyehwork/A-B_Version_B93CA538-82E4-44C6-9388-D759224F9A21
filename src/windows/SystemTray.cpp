#include "SystemTray.h"
#include "../utils/IconHelper.h"
#include <QApplication>
#include <QIcon>

SystemTray::SystemTray(QObject* parent) : QObject(parent) {
    m_trayIcon = new QSystemTrayIcon(this);
    
    // 使用应用图标作为托盘图标
    m_trayIcon->setIcon(QIcon(":/icons/app_icon.ico"));
    m_trayIcon->setToolTip("搜索主程序");

    m_menu = new QMenu();
    m_menu->setWindowFlags(m_menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    m_menu->setAttribute(Qt::WA_TranslucentBackground);
    
    m_menu->addAction(IconHelper::getIcon("search", "#aaaaaa", 18), "打开搜索窗口", this, &SystemTray::showWindow);
    m_menu->addSeparator();
    m_menu->addAction(IconHelper::getIcon("close", "#aaaaaa", 18), "退出程序", this, &SystemTray::quitApp);

    m_trayIcon->setContextMenu(m_menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            emit showWindow();
        }
    });
}

void SystemTray::show() {
    m_trayIcon->show();
}