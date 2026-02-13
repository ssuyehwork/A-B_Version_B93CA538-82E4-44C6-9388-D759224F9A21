#include "FramelessDialog.h"
#include "../utils/IconHelper.h"
#include "../utils/StringUtils.h"
#include <QGraphicsDropShadowEffect>
#include <QSettings>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPen>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QMenu>
#include <QCursor>

FramelessDialog::FramelessDialog(const QString& title, QWidget* parent) 
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Window) 
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setMinimumWidth(40);
    setWindowTitle(title);

    m_outerLayout = new QVBoxLayout(this);
    m_outerLayout->setContentsMargins(20, 20, 20, 20);

    m_container = new QWidget(this);
    m_container->setObjectName("DialogContainer");
    m_container->setAttribute(Qt::WA_StyledBackground);
    m_container->setStyleSheet(
        "#DialogContainer {"
        "  background-color: #1e1e1e;"
        "  border: 1px solid #333333;"
        "  border-radius: 12px;"
        "} " + StringUtils::getToolTipStyle()
    );
    m_outerLayout->addWidget(m_container);

    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(20);
    m_shadow->setXOffset(0);
    m_shadow->setYOffset(4);
    m_shadow->setColor(QColor(0, 0, 0, 120));
    m_container->setGraphicsEffect(m_shadow);

    m_mainLayout = new QVBoxLayout(m_container);
    m_mainLayout->setContentsMargins(0, 0, 0, 10); 
    m_mainLayout->setSpacing(0);

    auto* titleBar = new QWidget();
    titleBar->setObjectName("TitleBar");
    titleBar->setMinimumHeight(38);
    titleBar->setStyleSheet("background-color: transparent; border-bottom: 1px solid #2D2D2D;");
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(12, 0, 5, 0);
    titleLayout->setSpacing(8);

    auto* iconLabel = new QLabel();
    iconLabel->setPixmap(QIcon(":/icons/app_icon.ico").pixmap(16, 16));
    iconLabel->setStyleSheet("border: none; background: transparent;");
    titleLayout->addWidget(iconLabel);

    m_titleLabel = new QLabel(title);
    m_titleLabel->setStyleSheet("color: #888; font-size: 12px; font-weight: bold; border: none;");
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    m_btnPin = new QPushButton();
    m_btnPin->setObjectName("btnPin");
    m_btnPin->setFixedSize(28, 28);
    m_btnPin->setIconSize(QSize(18, 18));
    m_btnPin->setAutoDefault(false);
    m_btnPin->setCheckable(true);
    m_btnPin->setIcon(IconHelper::getIcon("pin_vertical", "#ffffff"));
    
    m_btnPin->blockSignals(true);
    m_btnPin->setChecked(m_isStayOnTop); 
    m_btnPin->blockSignals(false);
    m_btnPin->setStyleSheet(StringUtils::getToolTipStyle() + 
                          "QPushButton { border: none; background: transparent; border-radius: 4px; } "
                          "QPushButton:hover { background-color: rgba(255, 255, 255, 0.1); } "
                          "QPushButton:pressed { background-color: rgba(255, 255, 255, 0.2); } "
                          "QPushButton:checked { background-color: rgba(58, 144, 255, 0.3); }");
    m_btnPin->setToolTip(StringUtils::wrapToolTip("置顶"));
    connect(m_btnPin, &QPushButton::toggled, this, &FramelessDialog::toggleStayOnTop);
    titleLayout->addWidget(m_btnPin);

    m_minBtn = new QPushButton();
    m_minBtn->setObjectName("minBtn");
    m_minBtn->setFixedSize(28, 28);
    m_minBtn->setIconSize(QSize(18, 18));
    m_minBtn->setIcon(IconHelper::getIcon("minimize", "#888888"));
    m_minBtn->setAutoDefault(false);
    m_minBtn->setToolTip(StringUtils::wrapToolTip("最小化"));
    m_minBtn->setCursor(Qt::PointingHandCursor);
    m_minBtn->setStyleSheet(StringUtils::getToolTipStyle() + 
        "QPushButton { background: transparent; border: none; border-radius: 4px; } "
        "QPushButton:hover { background-color: rgba(255, 255, 255, 0.1); }"
    );
    connect(m_minBtn, &QPushButton::clicked, this, &QDialog::showMinimized);
    titleLayout->addWidget(m_minBtn);

    m_maxBtn = new QPushButton();
    m_maxBtn->setObjectName("maxBtn");
    m_maxBtn->setFixedSize(28, 28);
    m_maxBtn->setIconSize(QSize(16, 16));
    m_maxBtn->setIcon(IconHelper::getIcon("maximize", "#888888"));
    m_maxBtn->setAutoDefault(false);
    m_maxBtn->setToolTip(StringUtils::wrapToolTip("最大化"));
    m_maxBtn->setCursor(Qt::PointingHandCursor);
    m_maxBtn->setStyleSheet(StringUtils::getToolTipStyle() + 
        "QPushButton { background: transparent; border: none; border-radius: 4px; } "
        "QPushButton:hover { background-color: rgba(255, 255, 255, 0.1); }"
    );
    connect(m_maxBtn, &QPushButton::clicked, this, &FramelessDialog::toggleMaximize);
    titleLayout->addWidget(m_maxBtn);

    m_closeBtn = new QPushButton();
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFixedSize(28, 28);
    m_closeBtn->setIconSize(QSize(18, 18));
    m_closeBtn->setIcon(IconHelper::getIcon("close", "#888888"));
    m_closeBtn->setAutoDefault(false);
    m_closeBtn->setToolTip(StringUtils::wrapToolTip("关闭"));
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(StringUtils::getToolTipStyle() + 
        "QPushButton { background: transparent; border: none; border-radius: 4px; } "
        "QPushButton:hover { background-color: #E81123; }"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleLayout->addWidget(m_closeBtn);

    m_mainLayout->addWidget(titleBar);

    m_contentArea = new QWidget();
    m_contentArea->setObjectName("DialogContentArea");
    m_contentArea->setAttribute(Qt::WA_StyledBackground);
    m_contentArea->setStyleSheet("QWidget#DialogContentArea { background: transparent; border: none; }");
    m_mainLayout->addWidget(m_contentArea, 1);
}

void FramelessDialog::setStayOnTop(bool stay) {
    if (m_btnPin) m_btnPin->setChecked(stay);
}

void FramelessDialog::toggleStayOnTop(bool checked) {
    m_isStayOnTop = checked;
    saveWindowSettings();

    if (isVisible()) {
#ifdef Q_OS_WIN
        HWND hwnd = (HWND)winId();
        SetWindowPos(hwnd, checked ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
        Qt::WindowFlags f = windowFlags();
        if (checked) f |= Qt::WindowStaysOnTopHint;
        else f &= ~Qt::WindowStaysOnTopHint;
        setWindowFlags(f);
        show();
#endif
    }

    if (m_btnPin) {
        m_btnPin->setIcon(IconHelper::getIcon(checked ? "pin_vertical" : "pin_tilted", checked ? "#ffffff" : "#aaaaaa"));
    }
}

void FramelessDialog::toggleMaximize() {
    if (isMaximized()) {
        showNormal();
        m_maxBtn->setIcon(IconHelper::getIcon("maximize", "#888888"));
        m_maxBtn->setToolTip(StringUtils::wrapToolTip("最大化"));
    } else {
        showMaximized();
        m_maxBtn->setIcon(IconHelper::getIcon("restore", "#888888"));
        m_maxBtn->setToolTip(StringUtils::wrapToolTip("还原"));
    }
}

void FramelessDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMaximized()) {
            m_maxBtn->setIcon(IconHelper::getIcon("restore", "#888888"));
            m_maxBtn->setToolTip(StringUtils::wrapToolTip("还原"));
            
            m_outerLayout->setContentsMargins(0, 0, 0, 0);
            m_container->setStyleSheet(
                "#DialogContainer {"
                "  background-color: #1e1e1e;"
                "  border: none;"
                "  border-radius: 0px;"
                "} " + StringUtils::getToolTipStyle()
            );
            if (m_shadow) m_shadow->setEnabled(false);
        } else {
            m_maxBtn->setIcon(IconHelper::getIcon("maximize", "#888888"));
            m_maxBtn->setToolTip(StringUtils::wrapToolTip("最大化"));

            m_outerLayout->setContentsMargins(20, 20, 20, 20);
            m_container->setStyleSheet(
                "#DialogContainer {"
                "  background-color: #1e1e1e;"
                "  border: 1px solid #333333;"
                "  border-radius: 12px;"
                "} " + StringUtils::getToolTipStyle()
            );
            if (m_shadow) m_shadow->setEnabled(true);
        }
    }
    QDialog::changeEvent(event);
}

void FramelessDialog::showEvent(QShowEvent* event) {
    if (m_firstShow) {
        loadWindowSettings();
        m_firstShow = false;
    }

    QDialog::showEvent(event);
#ifdef Q_OS_WIN
    if (m_isStayOnTop) {
        HWND hwnd = (HWND)winId();
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
#else
    Qt::WindowFlags f = windowFlags();
    if (m_isStayOnTop) f |= Qt::WindowStaysOnTopHint;
    else f &= ~Qt::WindowStaysOnTopHint;
    if (windowFlags() != f) {
        setWindowFlags(f);
        show();
    }
#endif
}

void FramelessDialog::loadWindowSettings() {
    if (objectName().isEmpty()) return;
    QSettings settings("SearchTool_Standalone", "WindowStates");
    bool stay = settings.value(objectName() + "/StayOnTop", false).toBool();
    
    m_isStayOnTop = stay;
    if (m_btnPin) {
        m_btnPin->blockSignals(true);
        m_btnPin->setChecked(stay);
        m_btnPin->setIcon(IconHelper::getIcon(stay ? "pin_vertical" : "pin_tilted", stay ? "#ffffff" : "#aaaaaa"));
        m_btnPin->blockSignals(false);
    }
}

void FramelessDialog::saveWindowSettings() {
    if (objectName().isEmpty()) return;
    QSettings settings("SearchTool_Standalone", "WindowStates");
    settings.setValue(objectName() + "/StayOnTop", m_isStayOnTop);
}

FramelessDialog::ResizeEdge FramelessDialog::getEdge(const QPoint& pos) {
    int x = pos.x();
    int y = pos.y();
    int w = width();
    int h = height();
    int edge = None;

    // 考虑到阴影边距（20px），实际可视边缘在 20 像素处
    // 但为了方便拖动，我们检测可视容器（DialogContainer）的边缘
    int margin = 20; 
    int tolerance = 8; // 触发缩放的感应宽度

    if (x >= margin - tolerance && x <= margin + tolerance) edge |= Left;
    if (x >= w - margin - tolerance && x <= w - margin + tolerance) edge |= Right;
    if (y >= margin - tolerance && y <= margin + tolerance) edge |= Top;
    if (y >= h - margin - tolerance && y <= h - margin + tolerance) edge |= Bottom;

    return static_cast<ResizeEdge>(edge);
}

void FramelessDialog::updateCursor(ResizeEdge edge) {
    switch (edge) {
        case Top:
        case Bottom: setCursor(Qt::SizeVerCursor); break;
        case Left:
        case Right: setCursor(Qt::SizeHorCursor); break;
        case TopLeft:
        case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
        case TopRight:
        case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
        default: setCursor(Qt::ArrowCursor); break;
    }
}

void FramelessDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (!isMaximized()) {
            m_resizeEdge = getEdge(event->pos());
            if (m_resizeEdge != None) {
                m_isResizing = true;
            } else {
                m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            }
        } else {
            // 如果最大化了，只允许拖动，但实际上最大化通常不允许拖动，或者拖动后还原
            // 这里我们保持简单，最大化时不处理拖动（除非我们要实现拖动标题栏还原）
        }
        event->accept();
    }
}

void FramelessDialog::mouseMoveEvent(QMouseEvent* event) {
    if (isMaximized()) {
        QDialog::mouseMoveEvent(event);
        return;
    }

    if (m_isResizing) {
        QRect rect = geometry();
        QPoint globalPos = event->globalPosition().toPoint();
        
        int minW = minimumWidth();
        int minH = minimumHeight() > 0 ? minimumHeight() : 100;

        if (m_resizeEdge & Left) {
            int newWidth = rect.right() - globalPos.x();
            if (newWidth >= minW) rect.setLeft(globalPos.x());
        }
        if (m_resizeEdge & Right) {
            rect.setRight(globalPos.x());
        }
        if (m_resizeEdge & Top) {
            int newHeight = rect.bottom() - globalPos.y();
            if (newHeight >= minH) rect.setTop(globalPos.y());
        }
        if (m_resizeEdge & Bottom) {
            rect.setBottom(globalPos.y());
        }
        
        if (rect.width() >= minW && rect.height() >= minH) {
            setGeometry(rect);
        }
    } else if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPos);
    } else {
        updateCursor(getEdge(event->pos()));
    }
    event->accept();
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent* event) {
    m_isResizing = false;
    m_resizeEdge = None;
    updateCursor(None);
    QDialog::mouseReleaseEvent(event);
}

void FramelessDialog::leaveEvent(QEvent* event) {
    if (!m_isResizing) {
        updateCursor(None);
    }
    QDialog::leaveEvent(event);
}

void FramelessDialog::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
}

void FramelessDialog::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_W) {
        reject();
    } else {
        QDialog::keyPressEvent(event);
    }
}
