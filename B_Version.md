# 代码导出结果 - 20260213_100032

**项目路径**: `G:\C++\Filesearch\暂存\B_Version`

**文件总数**: 25

## 文件类型统计

- **cpp**: 24 个文件
- **cmake**: 1 个文件

---

## 文件: `src/widgets/ClickableLineEdit.h`

```cpp
#ifndef CLICKABLELINEEDIT_H
#define CLICKABLELINEEDIT_H

#include <QLineEdit>
#include <QMouseEvent>

class ClickableLineEdit : public QLineEdit {
    Q_OBJECT
public:
    using QLineEdit::QLineEdit;
signals:
    void doubleClicked();
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) emit doubleClicked();
        QLineEdit::mouseDoubleClickEvent(event);
    }
};

#endif // CLICKABLELINEEDIT_H
```

## 文件: `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)

project(SearchTool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg)

# 源码文件路径相对于本 CMakeLists.txt 所在目录
set(SOURCES
    src/main.cpp
    src/windows/SearchAppWindow.h
    src/windows/SearchAppWindow.cpp
    src/windows/FileSearchWindow.h
    src/windows/FileSearchWindow.cpp
    src/windows/KeywordSearchWindow.h
    src/windows/KeywordSearchWindow.cpp
    src/windows/SystemTray.h
    src/windows/SystemTray.cpp
    src/windows/FramelessDialog.h
    src/windows/FramelessDialog.cpp
    src/widgets/ClickableLineEdit.h
    src/utils/StringUtils.h
    src/utils/IconHelper.h
    src/utils/SvgIcons.h
    resources/resources.qrc
)

if(WIN32)
    # 确保 rc 文件路径正确
    list(APPEND SOURCES resources/app_icon.rc)
endif()

add_executable(SearchTool ${SOURCES})

# 包含路径设置
target_include_directories(SearchTool PRIVATE 
    src/windows
    src/utils
    src/widgets
    src
)

target_link_libraries(SearchTool PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(SearchTool PRIVATE user32 shell32 psapi dwmapi)
    set_target_properties(SearchTool PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
```

## 文件: `src/ui/FileSearchWindow.cpp`

```cpp
#include "FileSearchWindow.h"
#include "StringUtils.h"

#include "IconHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDirIterator>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QLabel>
#include <QProcess>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QToolTip>
#include <QSettings>
#include <QSplitter>
#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <functional>
#include <utility>
#include <QSet>
#include <QDateTime>

// ----------------------------------------------------------------------------
// 合并逻辑相关常量与辅助函数
// ----------------------------------------------------------------------------
static const QSet<QString> SUPPORTED_EXTENSIONS = {
    ".py", ".pyw", ".cpp", ".cc", ".cxx", ".c", ".h", ".hpp", ".hxx",
    ".java", ".js", ".jsx", ".ts", ".tsx", ".cs", ".go", ".rs", ".swift",
    ".kt", ".kts", ".php", ".rb", ".lua", ".r", ".m", ".scala", ".sh",
    ".bash", ".zsh", ".ps1", ".bat", ".cmd", ".html", ".htm", ".css",
    ".scss", ".sass", ".less", ".xml", ".svg", ".vue", ".json", ".yaml",
    ".yml", ".toml", ".ini", ".cfg", ".conf", ".env", ".properties",
    ".cmake", ".gradle", ".make", ".mk", ".dockerfile", ".md", ".markdown",
    ".txt", ".rst", ".qml", ".qrc", ".qss", ".ui", ".sql", ".graphql",
    ".gql", ".proto", ".asm", ".s", ".v", ".vh", ".vhdl", ".vhd"
};

static const QSet<QString> SPECIAL_FILENAMES = {
    "Makefile", "makefile", "Dockerfile", "dockerfile", "CMakeLists.txt",
    "Rakefile", "Gemfile", ".gitignore", ".dockerignore", ".editorconfig",
    ".eslintrc", ".prettierrc"
};

static QString getFileLanguage(const QString& filePath) {
    QFileInfo fi(filePath);
    QString basename = fi.fileName();
    QString ext = "." + fi.suffix().toLower();
    
    static const QMap<QString, QString> specialMap = {
        {"Makefile", "makefile"}, {"makefile", "makefile"},
        {"Dockerfile", "dockerfile"}, {"dockerfile", "dockerfile"},
        {"CMakeLists.txt", "cmake"}
    };
    if (specialMap.contains(basename)) return specialMap[basename];

    static const QMap<QString, QString> extMap = {
        {".py", "python"}, {".pyw", "python"}, {".cpp", "cpp"}, {".cc", "cpp"},
        {".cxx", "cpp"}, {".c", "c"}, {".h", "cpp"}, {".hpp", "cpp"},
        {".hxx", "cpp"}, {".java", "java"}, {".js", "javascript"},
        {".jsx", "jsx"}, {".ts", "typescript"}, {".tsx", "tsx"},
        {".cs", "csharp"}, {".go", "go"}, {".rs", "rust"}, {".swift", "swift"},
        {".kt", "kotlin"}, {".kts", "kotlin"}, {".php", "php"}, {".rb", "ruby"},
        {".lua", "lua"}, {".r", "r"}, {".m", "objectivec"}, {".scala", "scala"},
        {".sh", "bash"}, {".bash", "bash"}, {".zsh", "zsh"}, {".ps1", "powershell"},
        {".bat", "batch"}, {".cmd", "batch"}, {".html", "html"}, {".htm", "html"},
        {".css", "css"}, {".scss", "scss"}, {".sass", "sass"}, {".less", "less"},
        {".xml", "xml"}, {".svg", "svg"}, {".vue", "vue"}, {".json", "json"},
        {".yaml", "yaml"}, {".yml", "yaml"}, {".toml", "toml"}, {".ini", "ini"},
        {".cfg", "ini"}, {".conf", "conf"}, {".env", "bash"},
        {".properties", "properties"}, {".cmake", "cmake"}, {".gradle", "gradle"},
        {".make", "makefile"}, {".mk", "makefile"}, {".dockerfile", "dockerfile"},
        {".md", "markdown"}, {".markdown", "markdown"}, {".txt", "text"},
        {".rst", "restructuredtext"}, {".qml", "qml"}, {".qrc", "xml"},
        {".qss", "css"}, {".ui", "xml"}, {".sql", "sql"}, {".graphql", "graphql"},
        {".gql", "graphql"}, {".proto", "protobuf"}, {".asm", "asm"},
        {".s", "asm"}, {".v", "verilog"}, {".vh", "verilog"}, {".vhdl", "vhdl"},
        {".vhd", "vhdl"}
    };
    return extMap.value(ext, ext.mid(1).isEmpty() ? "text" : ext.mid(1));
}

static bool isSupportedFile(const QString& filePath) {
    QFileInfo fi(filePath);
    if (SPECIAL_FILENAMES.contains(fi.fileName())) return true;
    return SUPPORTED_EXTENSIONS.contains("." + fi.suffix().toLower());
}

// ----------------------------------------------------------------------------
// PathHistory 相关辅助类 (复刻 SearchHistoryPopup 逻辑)
// ----------------------------------------------------------------------------
class PathChip : public QFrame {
    Q_OBJECT
public:
    PathChip(const QString& text, QWidget* parent = nullptr) : QFrame(parent), m_text(text) {
        setAttribute(Qt::WA_StyledBackground);
        setCursor(Qt::PointingHandCursor);
        setObjectName("PathChip");
        
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 6, 10, 6);
        layout->setSpacing(10);
        
        auto* lbl = new QLabel(text);
        lbl->setStyleSheet("border: none; background: transparent; color: #DDD; font-size: 13px;");
        layout->addWidget(lbl);
        layout->addStretch();
        
        auto* btnDel = new QPushButton();
        btnDel->setIcon(IconHelper::getIcon("close", "#666", 16));
        btnDel->setIconSize(QSize(10, 10));
        btnDel->setFixedSize(16, 16);
        btnDel->setCursor(Qt::PointingHandCursor);
        btnDel->setStyleSheet(
            "QPushButton { background-color: transparent; border-radius: 4px; padding: 0px; }"
            "QPushButton:hover { background-color: #E74C3C; }"
        );
        
        connect(btnDel, &QPushButton::clicked, this, [this](){ emit deleted(m_text); });
        layout->addWidget(btnDel);

        setStyleSheet(
            "#PathChip { background-color: transparent; border: none; border-radius: 4px; }"
            "#PathChip:hover { background-color: #3E3E42; }"
        );
    }
    
    void mousePressEvent(QMouseEvent* e) override { 
        if(e->button() == Qt::LeftButton) emit clicked(m_text); 
        QFrame::mousePressEvent(e);
    }

signals:
    void clicked(const QString& text);
    void deleted(const QString& text);
private:
    QString m_text;
};

// ----------------------------------------------------------------------------
// Sidebar ListWidget subclass for Drag & Drop
// ----------------------------------------------------------------------------
class FileSidebarListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit FileSidebarListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setAcceptDrops(true);
    }
signals:
    void folderDropped(const QString& path);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
            event->acceptProposedAction();
        }
    }
    void dragMoveEvent(QDragMoveEvent* event) override {
        event->acceptProposedAction();
    }
    void dropEvent(QDropEvent* event) override {
        QString path;
        if (event->mimeData()->hasUrls()) {
            path = event->mimeData()->urls().at(0).toLocalFile();
        } else if (event->mimeData()->hasText()) {
            path = event->mimeData()->text();
        }
        
        if (!path.isEmpty() && QDir(path).exists()) {
            emit folderDropped(path);
            event->acceptProposedAction();
        }
    }
};

class FileSearchHistoryPopup : public QWidget {
    Q_OBJECT
public:
    enum Type { Path, Filename };

    explicit FileSearchHistoryPopup(FileSearchWindow* window, QLineEdit* edit, Type type) 
        : QWidget(window->window(), Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) 
    {
        m_window = window;
        m_edit = edit;
        m_type = type;
        setAttribute(Qt::WA_TranslucentBackground);
        
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(12, 12, 12, 12);
        
        auto* container = new QFrame();
        container->setObjectName("PopupContainer");
        container->setStyleSheet(
            "#PopupContainer { background-color: #252526; border: 1px solid #444; border-radius: 10px; }"
        );
        rootLayout->addWidget(container);

        auto* shadow = new QGraphicsDropShadowEffect(container);
        shadow->setBlurRadius(20); shadow->setXOffset(0); shadow->setYOffset(5);
        shadow->setColor(QColor(0, 0, 0, 120));
        container->setGraphicsEffect(shadow);

        auto* layout = new QVBoxLayout(container);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(10);

        auto* top = new QHBoxLayout();
        auto* icon = new QLabel();
        icon->setPixmap(IconHelper::getIcon("clock", "#888").pixmap(14, 14));
        icon->setStyleSheet("border: none; background: transparent;");
        top->addWidget(icon);

        auto* title = new QLabel(m_type == Path ? "最近扫描路径" : "最近搜索文件名");
        title->setStyleSheet("color: #888; font-weight: bold; font-size: 11px; background: transparent; border: none;");
        top->addWidget(title);
        top->addStretch();
        auto* clearBtn = new QPushButton("清空");
        clearBtn->setCursor(Qt::PointingHandCursor);
        clearBtn->setStyleSheet("QPushButton { background: transparent; color: #666; border: none; font-size: 11px; } QPushButton:hover { color: #E74C3C; }");
        connect(clearBtn, &QPushButton::clicked, [this](){
            if (m_type == Path) m_window->clearHistory();
            else m_window->clearSearchHistory();
            refreshUI();
        });
        top->addWidget(clearBtn);
        layout->addLayout(top);

        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet(
            "QScrollArea { background-color: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background-color: transparent; }"
        );
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        m_chipsWidget = new QWidget();
        m_chipsWidget->setStyleSheet("background-color: transparent;");
        m_vLayout = new QVBoxLayout(m_chipsWidget);
        m_vLayout->setContentsMargins(0, 0, 0, 0);
        m_vLayout->setSpacing(2);
        m_vLayout->addStretch();
        scroll->setWidget(m_chipsWidget);
        layout->addWidget(scroll);

        m_opacityAnim = new QPropertyAnimation(this, "windowOpacity");
        m_opacityAnim->setDuration(200);
    }

    void refreshUI() {
        QLayoutItem* item;
        while ((item = m_vLayout->takeAt(0))) {
            if(item->widget()) item->widget()->deleteLater();
            delete item;
        }
        m_vLayout->addStretch();
        
        QStringList history = (m_type == Path) ? m_window->getHistory() : m_window->getSearchHistory();
        if(history.isEmpty()) {
            auto* lbl = new QLabel("暂无历史记录");
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet("color: #555; font-style: italic; margin: 20px; border: none;");
            m_vLayout->insertWidget(0, lbl);
        } else {
            for(const QString& val : std::as_const(history)) {
                auto* chip = new PathChip(val);
                chip->setFixedHeight(32);
                connect(chip, &PathChip::clicked, this, [this](const QString& v){ 
                    if (m_type == Path) m_window->useHistoryPath(v);
                    else m_edit->setText(v);
                    close(); 
                });
                connect(chip, &PathChip::deleted, this, [this](const QString& v){ 
                    if (m_type == Path) m_window->removeHistoryEntry(v);
                    else m_window->removeSearchHistoryEntry(v);
                    refreshUI(); 
                });
                m_vLayout->insertWidget(m_vLayout->count() - 1, chip);
            }
        }
        
        int targetWidth = m_edit->width();
        // 统一高度为 410px，确保视觉一致性，不论记录多少（如同图一的效果）
        int contentHeight = 410;
        resize(targetWidth + 24, contentHeight);
    }

    void showAnimated() {
        refreshUI();
        QPoint pos = m_edit->mapToGlobal(QPoint(0, m_edit->height()));
        move(pos.x() - 12, pos.y() - 7);
        setWindowOpacity(0);
        show();
        m_opacityAnim->setStartValue(0);
        m_opacityAnim->setEndValue(1);
        m_opacityAnim->start();
    }

private:
    FileSearchWindow* m_window;
    QLineEdit* m_edit;
    Type m_type;
    QWidget* m_chipsWidget;
    QVBoxLayout* m_vLayout;
    QPropertyAnimation* m_opacityAnim;
};

// ----------------------------------------------------------------------------
// ScannerThread 实现
// ----------------------------------------------------------------------------
ScannerThread::ScannerThread(const QString& folderPath, QObject* parent)
    : QThread(parent), m_folderPath(folderPath) {}

void ScannerThread::stop() {
    m_isRunning = false;
    wait();
}

void ScannerThread::run() {
    int count = 0;
    if (m_folderPath.isEmpty() || !QDir(m_folderPath).exists()) {
        emit finished(0);
        return;
    }

    QStringList ignored = {".git", ".idea", "__pycache__", "node_modules", "$RECYCLE.BIN", "System Volume Information"};
    
    // 使用 std::function 实现递归扫描，支持目录剪枝
    std::function<void(const QString&)> scanDir = [&](const QString& currentPath) {
        if (!m_isRunning) return;

        QDir dir(currentPath);
        // 1. 获取当前目录下所有文件
        QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const auto& fi : std::as_const(files)) {
            if (!m_isRunning) return;
            bool hidden = fi.isHidden();
            // 在某些平台上，以 . 开头的文件可能没被标记为 hidden，但通常我们也视为隐性文件
            if (!hidden && fi.fileName().startsWith('.')) hidden = true;
            
            emit fileFound(fi.fileName(), fi.absoluteFilePath(), hidden);
            count++;
        }

        // 2. 获取子目录并递归 (排除忽略列表)
        QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const auto& di : std::as_const(subDirs)) {
            if (!m_isRunning) return;
            if (!ignored.contains(di.fileName())) {
                scanDir(di.absoluteFilePath());
            }
        }
    };

    scanDir(m_folderPath);
    emit finished(count);
}

// ----------------------------------------------------------------------------
// ResizeHandle 实现
// ----------------------------------------------------------------------------
ResizeHandle::ResizeHandle(QWidget* target, QWidget* parent) 
    : QWidget(parent), m_target(target) 
{
    setFixedSize(20, 20);
    setCursor(Qt::SizeFDiagCursor);
}

void ResizeHandle::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_startPos = event->globalPosition().toPoint();
        m_startSize = m_target->size();
        event->accept();
    }
}

void ResizeHandle::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->globalPosition().toPoint() - m_startPos;
        int newW = qMax(m_startSize.width() + delta.x(), 600);
        int newH = qMax(m_startSize.height() + delta.y(), 400);
        m_target->resize(newW, newH);
        event->accept();
    }
}

// ----------------------------------------------------------------------------
// FileSearchWindow 实现
// ----------------------------------------------------------------------------
FileSearchWindow::FileSearchWindow(QWidget* parent) 
    : FramelessDialog("搜索文件", parent)
{
    resize(1000, 680);
    setupStyles();
    initUI();
    loadFavorites();
    m_resizeHandle = new ResizeHandle(this, this);
    m_resizeHandle->raise();
}

FileSearchWindow::~FileSearchWindow() {
    if (m_scanThread) {
        m_scanThread->stop();
        m_scanThread->deleteLater();
    }
}

void FileSearchWindow::setupStyles() {
    // 1:1 复刻 Python 脚本中的 STYLESHEET
    setStyleSheet(R"(
        QWidget {
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 14px;
            color: #E0E0E0;
            outline: none;
        }
        QSplitter::handle {
            background-color: #333;
        }
        QListWidget {
            background-color: #252526; 
            border: 1px solid #333333;
            border-radius: 6px;
            padding: 4px;
        }
        QListWidget::item {
            height: 30px;
            padding-left: 8px;
            border-radius: 4px;
            color: #CCCCCC;
        }
        QListWidget::item:selected {
            background-color: #37373D;
            border-left: 3px solid #007ACC;
            color: #FFFFFF;
        }
        #SidebarList::item:selected {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #007ACC, stop:0.015 #007ACC, stop:0.016 #37373D, stop:1 #37373D);
            color: #FFFFFF;
            border-radius: 4px;
        }
        QListWidget::item:hover {
            background-color: #2A2D2E;
        }
        QLineEdit {
            background-color: #333333;
            border: 1px solid #444444;
            color: #FFFFFF;
            border-radius: 6px;
            padding: 8px;
            selection-background-color: #264F78;
        }
        QLineEdit:focus {
            border: 1px solid #007ACC;
            background-color: #2D2D2D;
        }
        #ActionBtn {
            background-color: #007ACC;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
        }
        #ActionBtn:hover {
            background-color: #0062A3;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
}

void FileSearchWindow::initUI() {
    auto* mainLayout = new QHBoxLayout(m_contentArea);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);

    // --- 左侧边栏 ---
    auto* sidebarWidget = new QWidget();
    auto* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(0, 0, 5, 0);
    sidebarLayout->setSpacing(10);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(5);
    auto* sidebarIcon = new QLabel();
    sidebarIcon->setPixmap(IconHelper::getIcon("folder", "#888").pixmap(14, 14));
    sidebarIcon->setStyleSheet("border: none; background: transparent;");
    headerLayout->addWidget(sidebarIcon);

    auto* sidebarHeader = new QLabel("收藏夹 (可拖入)");
    sidebarHeader->setStyleSheet("color: #888; font-weight: bold; font-size: 12px; border: none; background: transparent;");
    headerLayout->addWidget(sidebarHeader);
    headerLayout->addStretch();
    sidebarLayout->addLayout(headerLayout);

    auto* sidebar = new FileSidebarListWidget();
    m_sidebar = sidebar;
    m_sidebar->setObjectName("SidebarList");
    m_sidebar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebar->setMinimumWidth(200);
    m_sidebar->setDragEnabled(false);
    m_sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(sidebar, &FileSidebarListWidget::folderDropped, this, &FileSearchWindow::addFavorite);
    connect(m_sidebar, &QListWidget::itemClicked, this, &FileSearchWindow::onSidebarItemClicked);
    connect(m_sidebar, &QListWidget::customContextMenuRequested, this, &FileSearchWindow::showSidebarContextMenu);
    sidebarLayout->addWidget(m_sidebar);

    auto* btnAddFav = new QPushButton("收藏当前路径");
    btnAddFav->setFixedHeight(32);
    btnAddFav->setCursor(Qt::PointingHandCursor);
    btnAddFav->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #444; color: #AAA; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #3E3E42; color: #FFF; border-color: #666; }"
    );
    connect(btnAddFav, &QPushButton::clicked, this, [this](){
        QString p = m_pathInput->text().trimmed();
        if (QDir(p).exists()) addFavorite(p);
    });
    sidebarLayout->addWidget(btnAddFav);

    splitter->addWidget(sidebarWidget);

    // --- 右侧主区域 ---
    auto* rightWidget = new QWidget();
    auto* layout = new QVBoxLayout(rightWidget);
    layout->setContentsMargins(5, 0, 0, 0);
    layout->setSpacing(10);

    // 第一行：路径输入与浏览
    auto* pathLayout = new QHBoxLayout();
    m_pathInput = new QLineEdit();
    m_pathInput->setPlaceholderText("双击查看历史，或在此粘贴路径...");
    m_pathInput->setClearButtonEnabled(true);
    m_pathInput->installEventFilter(this);
    connect(m_pathInput, &QLineEdit::returnPressed, this, &FileSearchWindow::onPathReturnPressed);
    
    auto* btnScan = new QToolButton();
    btnScan->setIcon(IconHelper::getIcon("scan", "#1abc9c", 18));
    btnScan->setToolTip(StringUtils::wrapToolTip("开始扫描"));
    btnScan->setFixedSize(38, 38);
    btnScan->setCursor(Qt::PointingHandCursor);
    btnScan->setStyleSheet("QToolButton { border: 1px solid #444; background: #2D2D30; border-radius: 6px; }"
                           "QToolButton:hover { background-color: #3E3E42; border-color: #007ACC; }");
    connect(btnScan, &QToolButton::clicked, this, &FileSearchWindow::onPathReturnPressed);

    auto* btnBrowse = new QToolButton();
    btnBrowse->setObjectName("ActionBtn");
    btnBrowse->setIcon(IconHelper::getIcon("folder", "#ffffff", 18));
    btnBrowse->setToolTip(StringUtils::wrapToolTip("浏览文件夹"));
    btnBrowse->setFixedSize(38, 38);
    btnBrowse->setCursor(Qt::PointingHandCursor);
    connect(btnBrowse, &QToolButton::clicked, this, &FileSearchWindow::selectFolder);

    pathLayout->addWidget(m_pathInput);
    pathLayout->addWidget(btnScan);
    pathLayout->addWidget(btnBrowse);
    layout->addLayout(pathLayout);

    // 第二行：搜索过滤与后缀名
    auto* searchLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("输入文件名过滤...");
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->installEventFilter(this);
    connect(m_searchInput, &QLineEdit::textChanged, this, &FileSearchWindow::refreshList);
    connect(m_searchInput, &QLineEdit::returnPressed, this, [this](){
        addSearchHistoryEntry(m_searchInput->text().trimmed());
    });

    m_extInput = new QLineEdit();
    m_extInput->setPlaceholderText("后缀 (如 py)");
    m_extInput->setClearButtonEnabled(true);
    m_extInput->setFixedWidth(120);
    connect(m_extInput, &QLineEdit::textChanged, this, &FileSearchWindow::refreshList);

    searchLayout->addWidget(m_searchInput);
    searchLayout->addWidget(m_extInput);
    layout->addLayout(searchLayout);

    // 信息标签与显示隐藏文件勾选
    auto* infoLayout = new QHBoxLayout();
    m_infoLabel = new QLabel("等待操作...");
    m_infoLabel->setStyleSheet("color: #888888; font-size: 12px;");
    
    m_showHiddenCheck = new QCheckBox("显示隐性文件");
    m_showHiddenCheck->setStyleSheet(R"(
        QCheckBox { color: #888; font-size: 12px; spacing: 5px; }
        QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #444; border-radius: 3px; background: #2D2D30; }
        QCheckBox::indicator:checked { background-color: #007ACC; border-color: #007ACC; }
        QCheckBox::indicator:hover { border-color: #666; }
    )");
    connect(m_showHiddenCheck, &QCheckBox::toggled, this, &FileSearchWindow::refreshList);

    infoLayout->addWidget(m_infoLabel);
    infoLayout->addWidget(m_showHiddenCheck);
    infoLayout->addStretch();
    layout->addLayout(infoLayout);

    // 列表标题与复制全部按钮
    auto* listHeaderLayout = new QHBoxLayout();
    listHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto* listTitle = new QLabel("搜索结果");
    listTitle->setStyleSheet("color: #888; font-size: 11px; font-weight: bold; border: none; background: transparent;");
    
    auto* btnCopyAll = new QToolButton();
    btnCopyAll->setIcon(IconHelper::getIcon("copy", "#1abc9c", 14));
    btnCopyAll->setToolTip(StringUtils::wrapToolTip("复制全部搜索结果的路径"));
    btnCopyAll->setFixedSize(20, 20);
    btnCopyAll->setCursor(Qt::PointingHandCursor);
    btnCopyAll->setStyleSheet("QToolButton { border: none; background: transparent; padding: 2px; }"
                               "QToolButton:hover { background-color: #3E3E42; border-radius: 4px; }");
    connect(btnCopyAll, &QToolButton::clicked, this, [this](){
        if (m_fileList->count() == 0) {
            QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 结果列表为空</b>"), this, {}, 2000);
            return;
        }
        QStringList paths;
        for (int i = 0; i < m_fileList->count(); ++i) {
            QString p = m_fileList->item(i)->data(Qt::UserRole).toString();
            if (!p.isEmpty()) paths << p;
        }
        if (paths.isEmpty()) return;
        QApplication::clipboard()->setText(paths.join("\n"));
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#2ecc71;'>✔ 已复制全部搜索结果</b>"), this, {}, 2000);
    });

    listHeaderLayout->addWidget(listTitle);
    listHeaderLayout->addStretch();
    listHeaderLayout->addWidget(btnCopyAll);
    layout->addLayout(listHeaderLayout);

    // 文件列表
    m_fileList = new QListWidget();
    m_fileList->setObjectName("FileList");
    m_fileList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fileList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileList, &QListWidget::customContextMenuRequested, this, &FileSearchWindow::showFileContextMenu);
    
    // 快捷键支持
    auto* actionSelectAll = new QAction(this);
    actionSelectAll->setShortcut(QKeySequence("Ctrl+A"));
    actionSelectAll->setShortcutContext(Qt::WidgetShortcut);
    connect(actionSelectAll, &QAction::triggered, [this](){ m_fileList->selectAll(); });
    m_fileList->addAction(actionSelectAll);

    auto* actionCopy = new QAction(this);
    actionCopy->setShortcut(QKeySequence("Ctrl+C"));
    actionCopy->setShortcutContext(Qt::WidgetShortcut);
    connect(actionCopy, &QAction::triggered, this, [this](){ copySelectedFiles(); });
    m_fileList->addAction(actionCopy);

    auto* actionDelete = new QAction(this);
    actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(actionDelete, &QAction::triggered, this, [this](){ onDeleteFile(); });
    m_fileList->addAction(actionDelete);

    layout->addWidget(m_fileList);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(1, 1);
}

void FileSearchWindow::selectFolder() {
    QString d = QFileDialog::getExistingDirectory(this, "选择文件夹");
    if (!d.isEmpty()) {
        m_pathInput->setText(d);
        startScan(d);
    }
}

void FileSearchWindow::onPathReturnPressed() {
    QString p = m_pathInput->text().trimmed();
    if (QDir(p).exists()) {
        startScan(p);
    } else {
        m_infoLabel->setText("路径不存在");
        m_pathInput->setStyleSheet("border: 1px solid #FF3333;");
    }
}

void FileSearchWindow::startScan(const QString& path) {
    m_pathInput->setStyleSheet("");
    if (m_scanThread) {
        m_scanThread->stop();
        m_scanThread->deleteLater();
    }

    m_fileList->clear();
    m_filesData.clear();
    m_visibleCount = 0;
    m_hiddenCount = 0;
    m_infoLabel->setText("正在扫描: " + path);

    m_scanThread = new ScannerThread(path, this);
    connect(m_scanThread, &ScannerThread::fileFound, this, &FileSearchWindow::onFileFound);
    connect(m_scanThread, &ScannerThread::finished, this, &FileSearchWindow::onScanFinished);
    m_scanThread->start();
}

void FileSearchWindow::onFileFound(const QString& name, const QString& path, bool isHidden) {
    m_filesData.append({name, path, isHidden});
    if (isHidden) m_hiddenCount++;
    else m_visibleCount++;

    if (m_filesData.size() % 300 == 0) {
        m_infoLabel->setText(QString("已发现 %1 个文件 (可见:%2 隐性:%3)...").arg(m_filesData.size()).arg(m_visibleCount).arg(m_hiddenCount));
    }
}

void FileSearchWindow::onScanFinished(int count) {
    m_infoLabel->setText(QString("扫描结束，共 %1 个文件 (可见:%2 隐性:%3)").arg(count).arg(m_visibleCount).arg(m_hiddenCount));
    addHistoryEntry(m_pathInput->text().trimmed());
    
    // 按文件名排序 (不按目录)
    std::sort(m_filesData.begin(), m_filesData.end(), [](const FileData& a, const FileData& b){
        return a.name.localeAwareCompare(b.name) < 0;
    });

    refreshList();
}

void FileSearchWindow::refreshList() {
    m_fileList->clear();
    QString txt = m_searchInput->text().toLower();
    QString ext = m_extInput->text().toLower().trimmed();
    if (ext.startsWith(".")) ext = ext.mid(1);

    bool showHidden = m_showHiddenCheck->isChecked();

    int limit = 500;
    int shown = 0;

    for (const auto& data : std::as_const(m_filesData)) {
        if (!showHidden && data.isHidden) continue;
        if (!ext.isEmpty() && !data.name.toLower().endsWith("." + ext)) continue;
        if (!txt.isEmpty() && !data.name.toLower().contains(txt)) continue;

        auto* item = new QListWidgetItem(data.name);
        item->setData(Qt::UserRole, data.path);
        item->setToolTip(StringUtils::wrapToolTip(data.path));
        m_fileList->addItem(item);
        
        shown++;
        if (shown >= limit) {
            auto* warn = new QListWidgetItem("--- 结果过多，仅显示前 500 条 ---");
            warn->setForeground(QColor(255, 170, 0));
            warn->setTextAlignment(Qt::AlignCenter);
            warn->setFlags(Qt::NoItemFlags);
            m_fileList->addItem(warn);
            break;
        }
    }
}

void FileSearchWindow::showFileContextMenu(const QPoint& pos) {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        auto* item = m_fileList->itemAt(pos);
        if (item) {
            item->setSelected(true);
            selectedItems << item;
        }
    }

    if (selectedItems.isEmpty()) return;

    QStringList paths;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) paths << p;
    }

    if (paths.isEmpty()) return;

    QMenu menu(this);
    menu.setStyleSheet("QMenu { background-color: #2D2D30; border: 1px solid #444; color: #EEE; } QMenu::item:selected { background-color: #3E3E42; }");
    
    if (selectedItems.size() == 1) {
        QString filePath = paths.first();
        menu.addAction(IconHelper::getIcon("folder", "#F1C40F"), "定位文件夹", [filePath](){
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
        });
        menu.addAction(IconHelper::getIcon("search", "#4A90E2"), "定位文件", [filePath](){
#ifdef Q_OS_WIN
            QStringList args;
            args << "/select," << QDir::toNativeSeparators(filePath);
            QProcess::startDetached("explorer.exe", args);
#endif
        });
        menu.addAction(IconHelper::getIcon("edit", "#3498DB"), "编辑", [this](){ onEditFile(); });
        menu.addSeparator();
    }

    QString copyPathText = selectedItems.size() > 1 ? "复制选中路径" : "复制完整路径";
    menu.addAction(IconHelper::getIcon("copy", "#2ECC71"), copyPathText, [paths](){
        QApplication::clipboard()->setText(paths.join("\n"));
    });

    QString copyFileText = selectedItems.size() > 1 ? "复制选中文件" : "复制文件";
    menu.addAction(IconHelper::getIcon("file", "#4A90E2"), copyFileText, [this](){ copySelectedFiles(); });

    menu.addAction(IconHelper::getIcon("merge", "#3498DB"), "合并选中内容", [this](){ onMergeSelectedFiles(); });

    menu.addSeparator();
    menu.addAction(IconHelper::getIcon("cut", "#E67E22"), "剪切", [this](){ onCutFile(); });
    menu.addAction(IconHelper::getIcon("trash", "#E74C3C"), "删除", [this](){ onDeleteFile(); });

    menu.exec(m_fileList->mapToGlobal(pos));
}

void FileSearchWindow::onEditFile() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    QStringList paths;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) paths << p;
    }
    if (paths.isEmpty()) return;

    QSettings settings("SearchTool", "ExternalEditor");
    QString editorPath = settings.value("EditorPath").toString();

    // 尝试寻找 Notepad++
    if (editorPath.isEmpty() || !QFile::exists(editorPath)) {
        QStringList commonPaths = {
            "C:/Program Files/Notepad++/notepad++.exe",
            "C:/Program Files (x86)/Notepad++/notepad++.exe"
        };
        for (const QString& p : commonPaths) {
            if (QFile::exists(p)) {
                editorPath = p;
                break;
            }
        }
    }

    // 如果还没找到，让用户选择
    if (editorPath.isEmpty() || !QFile::exists(editorPath)) {
        editorPath = QFileDialog::getOpenFileName(this, "选择编辑器 (推荐 Notepad++)", "C:/Program Files", "Executable (*.exe)");
        if (editorPath.isEmpty()) return;
        settings.setValue("EditorPath", editorPath);
    }

    for (const QString& filePath : paths) {
        QProcess::startDetached(editorPath, { QDir::toNativeSeparators(filePath) });
    }
}

void FileSearchWindow::copySelectedFiles() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    QList<QUrl> urls;
    QStringList paths;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) {
            urls << QUrl::fromLocalFile(p);
            paths << p;
        }
    }
    if (urls.isEmpty()) return;

    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(urls);
    mimeData->setText(paths.join("\n"));

    QApplication::clipboard()->setMimeData(mimeData);

    QString msg = selectedItems.size() > 1 ? QString("✔ 已复制 %1 个文件").arg(selectedItems.size()) : "✔ 已复制到剪贴板";
    QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>%1</b>").arg(msg)), this);
}

void FileSearchWindow::onCutFile() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    QList<QUrl> urls;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) urls << QUrl::fromLocalFile(p);
    }
    if (urls.isEmpty()) return;

    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(urls);
    
#ifdef Q_OS_WIN
    // 设置 Preferred DropEffect 为 2 (DROPEFFECT_MOVE)，通知资源管理器这是“剪切”操作
    QByteArray data;
    data.resize(4);
    data[0] = 2; // DROPEFFECT_MOVE
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    mimeData->setData("Preferred DropEffect", data);
#endif

    QApplication::clipboard()->setMimeData(mimeData);

    QString msg = selectedItems.size() > 1 ? QString("✔ 已剪切 %1 个文件").arg(selectedItems.size()) : "✔ 已剪切到剪贴板";
    QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>%1</b>").arg(msg)), this);
}

void FileSearchWindow::onDeleteFile() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    int successCount = 0;
    for (auto* item : std::as_const(selectedItems)) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (filePath.isEmpty()) continue;

        if (QFile::moveToTrash(filePath)) {
            successCount++;
            // 从内存数据中移除
            for (int i = 0; i < m_filesData.size(); ++i) {
                if (m_filesData[i].path == filePath) {
                    m_filesData.removeAt(i);
                    break;
                }
            }
            delete item; // 从界面移除 (QListWidget 负责管理内存)
        }
    }

    if (successCount > 0) {
        QString msg = selectedItems.size() > 1 ? QString("✔ %1 个文件已移至回收站").arg(successCount) : "✔ 文件已移至回收站";
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>%1</b>").arg(msg)), this);
        m_infoLabel->setText(msg);
    } else if (!selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color: #e74c3c;'>✖ 无法删除文件，请检查是否被占用</b>"), this);
    }
}

void FileSearchWindow::onMergeFiles(const QStringList& filePaths, const QString& rootPath) {
    if (filePaths.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 没有可合并的文件</b>"), this, {}, 2000);
        return;
    }

    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString outName = QString("%1_code_export.md").arg(ts);
    QString outPath = QDir(rootPath).filePath(outName);

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 无法创建输出文件</b>"), this, {}, 2000);
        return;
    }

    QTextStream out(&outFile);
    out.setEncoding(QStringConverter::Utf8);

    out << "# 代码导出结果 - " << ts << "\n\n";
    out << "**项目路径**: `" << rootPath << "`\n\n";
    out << "**文件总数**: " << filePaths.size() << "\n\n";

    QMap<QString, int> fileStats;
    for (const QString& fp : filePaths) {
        QString lang = getFileLanguage(fp);
        fileStats[lang]++;
    }

    out << "## 文件类型统计\n\n";
    QStringList langs = fileStats.keys();
    std::sort(langs.begin(), langs.end(), [&](const QString& a, const QString& b){
        return fileStats.value(a) > fileStats.value(b);
    });
    for (const QString& lang : std::as_const(langs)) {
        out << "- **" << lang << "**: " << fileStats.value(lang) << " 个文件\n";
    }
    out << "\n---\n\n";

    for (const QString& fp : filePaths) {
        QString relPath = QDir(rootPath).relativeFilePath(fp);
        QString lang = getFileLanguage(fp);

        out << "
```

## 文件: `src/windows/FileSearchWindow.cpp`

```cpp
#include "FileSearchWindow.h"
#include "../utils/StringUtils.h"

#include "../utils/IconHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDirIterator>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QLabel>
#include <QProcess>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QToolTip>
#include <QSettings>
#include <QSplitter>
#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <functional>
#include <utility>
#include <QSet>
#include <QDateTime>

// ----------------------------------------------------------------------------
// 合并逻辑相关常量与辅助函数
// ----------------------------------------------------------------------------
static const QSet<QString> SUPPORTED_EXTENSIONS = {
    ".py", ".pyw", ".cpp", ".cc", ".cxx", ".c", ".h", ".hpp", ".hxx",
    ".java", ".js", ".jsx", ".ts", ".tsx", ".cs", ".go", ".rs", ".swift",
    ".kt", ".kts", ".php", ".rb", ".lua", ".r", ".m", ".scala", ".sh",
    ".bash", ".zsh", ".ps1", ".bat", ".cmd", ".html", ".htm", ".css",
    ".scss", ".sass", ".less", ".xml", ".svg", ".vue", ".json", ".yaml",
    ".yml", ".toml", ".ini", ".cfg", ".conf", ".env", ".properties",
    ".cmake", ".gradle", ".make", ".mk", ".dockerfile", ".md", ".markdown",
    ".txt", ".rst", ".qml", ".qrc", ".qss", ".ui", ".sql", ".graphql",
    ".gql", ".proto", ".asm", ".s", ".v", ".vh", ".vhdl", ".vhd"
};

static const QSet<QString> SPECIAL_FILENAMES = {
    "Makefile", "makefile", "Dockerfile", "dockerfile", "CMakeLists.txt",
    "Rakefile", "Gemfile", ".gitignore", ".dockerignore", ".editorconfig",
    ".eslintrc", ".prettierrc"
};

static QString getFileLanguage(const QString& filePath) {
    QFileInfo fi(filePath);
    QString basename = fi.fileName();
    QString ext = "." + fi.suffix().toLower();
    
    static const QMap<QString, QString> specialMap = {
        {"Makefile", "makefile"}, {"makefile", "makefile"},
        {"Dockerfile", "dockerfile"}, {"dockerfile", "dockerfile"},
        {"CMakeLists.txt", "cmake"}
    };
    if (specialMap.contains(basename)) return specialMap[basename];

    static const QMap<QString, QString> extMap = {
        {".py", "python"}, {".pyw", "python"}, {".cpp", "cpp"}, {".cc", "cpp"},
        {".cxx", "cpp"}, {".c", "c"}, {".h", "cpp"}, {".hpp", "cpp"},
        {".hxx", "cpp"}, {".java", "java"}, {".js", "javascript"},
        {".jsx", "jsx"}, {".ts", "typescript"}, {".tsx", "tsx"},
        {".cs", "csharp"}, {".go", "go"}, {".rs", "rust"}, {".swift", "swift"},
        {".kt", "kotlin"}, {".kts", "kotlin"}, {".php", "php"}, {".rb", "ruby"},
        {".lua", "lua"}, {".r", "r"}, {".m", "objectivec"}, {".scala", "scala"},
        {".sh", "bash"}, {".bash", "bash"}, {".zsh", "zsh"}, {".ps1", "powershell"},
        {".bat", "batch"}, {".cmd", "batch"}, {".html", "html"}, {".htm", "html"},
        {".css", "css"}, {".scss", "scss"}, {".sass", "sass"}, {".less", "less"},
        {".xml", "xml"}, {".svg", "svg"}, {".vue", "vue"}, {".json", "json"},
        {".yaml", "yaml"}, {".yml", "yaml"}, {".toml", "toml"}, {".ini", "ini"},
        {".cfg", "ini"}, {".conf", "conf"}, {".env", "bash"},
        {".properties", "properties"}, {".cmake", "cmake"}, {".gradle", "gradle"},
        {".make", "makefile"}, {".mk", "makefile"}, {".dockerfile", "dockerfile"},
        {".md", "markdown"}, {".markdown", "markdown"}, {".txt", "text"},
        {".rst", "restructuredtext"}, {".qml", "qml"}, {".qrc", "xml"},
        {".qss", "css"}, {".ui", "xml"}, {".sql", "sql"}, {".graphql", "graphql"},
        {".gql", "graphql"}, {".proto", "protobuf"}, {".asm", "asm"},
        {".s", "asm"}, {".v", "verilog"}, {".vh", "verilog"}, {".vhdl", "vhdl"},
        {".vhd", "vhdl"}
    };
    return extMap.value(ext, ext.mid(1).isEmpty() ? "text" : ext.mid(1));
}

static bool isSupportedFile(const QString& filePath) {
    QFileInfo fi(filePath);
    if (SPECIAL_FILENAMES.contains(fi.fileName())) return true;
    return SUPPORTED_EXTENSIONS.contains("." + fi.suffix().toLower());
}

/**
 * @brief 自定义列表项，支持置顶排序逻辑
 */
class FavoriteItem : public QListWidgetItem {
public:
    using QListWidgetItem::QListWidgetItem;
    bool operator<(const QListWidgetItem &other) const override {
        bool thisPinned = data(Qt::UserRole + 1).toBool();
        bool otherPinned = other.data(Qt::UserRole + 1).toBool();
        if (thisPinned != otherPinned) return thisPinned; // true < false -> 置顶项排在前面
        return text().localeAwareCompare(other.text()) < 0;
    }
};

// ----------------------------------------------------------------------------
// PathHistory 相关辅助类 (复刻 SearchHistoryPopup 逻辑)
// ----------------------------------------------------------------------------
class PathChip : public QFrame {
    Q_OBJECT
public:
    PathChip(const QString& text, QWidget* parent = nullptr) : QFrame(parent), m_text(text) {
        setAttribute(Qt::WA_StyledBackground);
        setCursor(Qt::PointingHandCursor);
        setObjectName("PathChip");
        
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 6, 10, 6);
        layout->setSpacing(10);
        
        auto* lbl = new QLabel(text);
        lbl->setStyleSheet("border: none; background: transparent; color: #DDD; font-size: 13px;");
        layout->addWidget(lbl);
        layout->addStretch();
        
        auto* btnDel = new QPushButton();
        btnDel->setIcon(IconHelper::getIcon("close", "#666", 16));
        btnDel->setIconSize(QSize(10, 10));
        btnDel->setFixedSize(16, 16);
        btnDel->setCursor(Qt::PointingHandCursor);
        btnDel->setStyleSheet(
            "QPushButton { background-color: transparent; border-radius: 4px; padding: 0px; }"
            "QPushButton:hover { background-color: #E74C3C; }"
        );
        
        connect(btnDel, &QPushButton::clicked, this, [this](){ emit deleted(m_text); });
        layout->addWidget(btnDel);

        setStyleSheet(
            "#PathChip { background-color: transparent; border: none; border-radius: 4px; }"
            "#PathChip:hover { background-color: #3E3E42; }"
        );
    }
    
    void mousePressEvent(QMouseEvent* e) override { 
        if(e->button() == Qt::LeftButton) emit clicked(m_text); 
        QFrame::mousePressEvent(e);
    }

signals:
    void clicked(const QString& text);
    void deleted(const QString& text);
private:
    QString m_text;
};

// ----------------------------------------------------------------------------
// Sidebar ListWidget subclass for Drag & Drop
// ----------------------------------------------------------------------------
class FileSidebarListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit FileSidebarListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setAcceptDrops(true);
    }
signals:
    void folderDropped(const QString& path);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
            event->acceptProposedAction();
        }
    }
    void dragMoveEvent(QDragMoveEvent* event) override {
        event->acceptProposedAction();
    }
    void dropEvent(QDropEvent* event) override {
        QString path;
        if (event->mimeData()->hasUrls()) {
            path = event->mimeData()->urls().at(0).toLocalFile();
        } else if (event->mimeData()->hasText()) {
            path = event->mimeData()->text();
        }
        
        if (!path.isEmpty() && QDir(path).exists()) {
            emit folderDropped(path);
            event->acceptProposedAction();
        }
    }
};

class FileSearchHistoryPopup : public QWidget {
    Q_OBJECT
public:
    enum Type { Path, Filename, Extension };

    explicit FileSearchHistoryPopup(FileSearchWidget* searchWidget, QLineEdit* edit, Type type) 
        : QWidget(searchWidget->window(), Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) 
    {
        m_searchWidget = searchWidget;
        m_edit = edit;
        m_type = type;
        setAttribute(Qt::WA_TranslucentBackground);
        
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(12, 12, 12, 12);
        
        auto* container = new QFrame();
        container->setObjectName("PopupContainer");
        container->setStyleSheet(
            "#PopupContainer { background-color: #252526; border: 1px solid #444; border-radius: 10px; }"
        );
        rootLayout->addWidget(container);

        auto* shadow = new QGraphicsDropShadowEffect(container);
        shadow->setBlurRadius(20); shadow->setXOffset(0); shadow->setYOffset(5);
        shadow->setColor(QColor(0, 0, 0, 120));
        container->setGraphicsEffect(shadow);

        auto* layout = new QVBoxLayout(container);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(10);

        auto* top = new QHBoxLayout();
        QString titleStr = "最近扫描路径";
        if (m_type == Filename) titleStr = "最近搜索文件名";
        else if (m_type == Extension) titleStr = "最近搜索后缀";

        auto* icon = new QLabel();
        icon->setPixmap(IconHelper::getIcon("clock", "#888").pixmap(14, 14));
        icon->setStyleSheet("border: none; background: transparent;");
        icon->setToolTip(StringUtils::wrapToolTip(titleStr));
        top->addWidget(icon);

        top->addStretch();

        auto* clearBtn = new QPushButton();
        clearBtn->setIcon(IconHelper::getIcon("trash", "#666", 14));
        clearBtn->setIconSize(QSize(14, 14));
        clearBtn->setFixedSize(20, 20);
        clearBtn->setCursor(Qt::PointingHandCursor);
        clearBtn->setToolTip(StringUtils::wrapToolTip("清空历史记录"));
        clearBtn->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background-color: rgba(231, 76, 60, 0.2); }");
        connect(clearBtn, &QPushButton::clicked, [this](){
            if (m_type == Path) m_searchWidget->clearHistory();
            else if (m_type == Filename) m_searchWidget->clearSearchHistory();
            else if (m_type == Extension) m_searchWidget->clearExtHistory();
            refreshUI();
        });
        top->addWidget(clearBtn);
        layout->addLayout(top);

        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet(
            "QScrollArea { background-color: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background-color: transparent; }"
        );
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        m_chipsWidget = new QWidget();
        m_chipsWidget->setStyleSheet("background-color: transparent;");
        m_vLayout = new QVBoxLayout(m_chipsWidget);
        m_vLayout->setContentsMargins(0, 0, 0, 0);
        m_vLayout->setSpacing(2);
        m_vLayout->addStretch();
        scroll->setWidget(m_chipsWidget);
        layout->addWidget(scroll);

        m_opacityAnim = new QPropertyAnimation(this, "windowOpacity");
        m_opacityAnim->setDuration(200);
    }

    void refreshUI() {
        QLayoutItem* item;
        while ((item = m_vLayout->takeAt(0))) {
            if(item->widget()) item->widget()->deleteLater();
            delete item;
        }
        m_vLayout->addStretch();
        
        QStringList history;
        if (m_type == Path) history = m_searchWidget->getHistory();
        else if (m_type == Filename) history = m_searchWidget->getSearchHistory();
        else if (m_type == Extension) history = m_searchWidget->getExtHistory();

        if(history.isEmpty()) {
            auto* lbl = new QLabel("暂无历史记录");
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet("color: #555; font-style: italic; margin: 20px; border: none;");
            m_vLayout->insertWidget(0, lbl);
        } else {
            for(const QString& val : std::as_const(history)) {
                auto* chip = new PathChip(val);
                chip->setFixedHeight(32);
                connect(chip, &PathChip::clicked, this, [this](const QString& v){ 
                    if (m_type == Path) m_searchWidget->useHistoryPath(v);
                    else m_edit->setText(v);
                    close(); 
                });
                connect(chip, &PathChip::deleted, this, [this](const QString& v){ 
                    if (m_type == Path) m_searchWidget->removeHistoryEntry(v);
                    else if (m_type == Filename) m_searchWidget->removeSearchHistoryEntry(v);
                    else if (m_type == Extension) m_searchWidget->removeExtHistoryEntry(v);
                    refreshUI(); 
                });
                m_vLayout->insertWidget(m_vLayout->count() - 1, chip);
            }
        }
        
        int targetWidth = m_edit->width();
        int contentHeight = 410;
        setFixedWidth(targetWidth + 24);
        resize(targetWidth + 24, contentHeight);
    }

    void showAnimated() {
        refreshUI();
        QPoint pos = m_edit->mapToGlobal(QPoint(0, m_edit->height()));
        move(pos.x() - 12, pos.y() - 7);
        setWindowOpacity(0);
        show();
        m_opacityAnim->setStartValue(0);
        m_opacityAnim->setEndValue(1);
        m_opacityAnim->start();
    }

private:
    FileSearchWidget* m_searchWidget;
    QLineEdit* m_edit;
    Type m_type;
    QWidget* m_chipsWidget;
    QVBoxLayout* m_vLayout;
    QPropertyAnimation* m_opacityAnim;
};

// ----------------------------------------------------------------------------
// ScannerThread 实现
// ----------------------------------------------------------------------------
ScannerThread::ScannerThread(const QString& folderPath, QObject* parent)
    : QThread(parent), m_folderPath(folderPath) {}

void ScannerThread::stop() {
    m_isRunning = false;
    wait();
}

void ScannerThread::run() {
    int count = 0;
    if (m_folderPath.isEmpty() || !QDir(m_folderPath).exists()) {
        emit finished(0);
        return;
    }

    QStringList ignored = {".git", ".idea", "__pycache__", "node_modules", "$RECYCLE.BIN", "System Volume Information"};
    
    std::function<void(const QString&)> scanDir = [&](const QString& currentPath) {
        if (!m_isRunning) return;

        QDir dir(currentPath);
        QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const auto& fi : std::as_const(files)) {
            if (!m_isRunning) return;
            bool hidden = fi.isHidden();
            if (!hidden && fi.fileName().startsWith('.')) hidden = true;
            
            emit fileFound(fi.fileName(), fi.absoluteFilePath(), hidden);
            count++;
        }

        QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const auto& di : std::as_const(subDirs)) {
            if (!m_isRunning) return;
            if (!ignored.contains(di.fileName())) {
                scanDir(di.absoluteFilePath());
            }
        }
    };

    scanDir(m_folderPath);
    emit finished(count);
}


// ----------------------------------------------------------------------------
// FileSearchWidget 实现
// ----------------------------------------------------------------------------
FileSearchWidget::FileSearchWidget(QWidget* parent) : QWidget(parent) {
    setupStyles();
    initUI();
    loadFavorites();
    loadFileFavorites();
}

FileSearchWidget::~FileSearchWidget() {
    if (m_scanThread) {
        m_scanThread->stop();
        m_scanThread->deleteLater();
    }
}

void FileSearchWidget::setupStyles() {
    setStyleSheet(R"(
        QWidget {
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 14px;
            color: #E0E0E0;
            outline: none;
        }
        QSplitter::handle {
            background-color: #333;
        }
        QListWidget {
            background-color: #252526; 
            border: 1px solid #333333;
            border-radius: 6px;
            padding: 4px;
        }
        QListWidget::item {
            height: 30px;
            padding-left: 8px;
            border-radius: 4px;
            color: #CCCCCC;
        }
        QListWidget::item:selected {
            background-color: #37373D;
            border-left: 3px solid #007ACC;
            color: #FFFFFF;
        }
        QListWidget::item:hover {
            background-color: #2A2D2E;
        }
        QLineEdit {
            background-color: #333333;
            border: 1px solid #444444;
            color: #FFFFFF;
            border-radius: 6px;
            padding: 8px;
            selection-background-color: #264F78;
        }
        QLineEdit:focus {
            border: 1px solid #007ACC;
            background-color: #2D2D2D;
        }
        #ActionBtn {
            background-color: #007ACC;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
        }
        #ActionBtn:hover {
            background-color: #0062A3;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
}

void FileSearchWidget::initUI() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);

    // --- 左侧边栏 (目录收藏) ---
    auto* sidebarWidget = new QWidget();
    auto* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(0, 0, 5, 0);
    sidebarLayout->setSpacing(10);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(5);
    auto* sidebarIcon = new QLabel();
    sidebarIcon->setPixmap(IconHelper::getIcon("folder", "#888").pixmap(14, 14));
    sidebarIcon->setStyleSheet("border: none; background: transparent;");
    headerLayout->addWidget(sidebarIcon);

    auto* sidebarHeader = new QLabel("收藏夹 (可拖入)");
    sidebarHeader->setStyleSheet("color: #888; font-weight: bold; font-size: 12px; border: none; background: transparent;");
    headerLayout->addWidget(sidebarHeader);
    headerLayout->addStretch();
    sidebarLayout->addLayout(headerLayout);

    auto* sidebar = new FileSidebarListWidget();
    m_sidebar = sidebar;
    m_sidebar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebar->setMinimumWidth(200);
    m_sidebar->setDragEnabled(false);
    m_sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(sidebar, &FileSidebarListWidget::folderDropped, this, [this](const QString& path){ addFavorite(path); });
    connect(m_sidebar, &QListWidget::itemClicked, this, &FileSearchWidget::onSidebarItemClicked);
    connect(m_sidebar, &QListWidget::customContextMenuRequested, this, &FileSearchWidget::showSidebarContextMenu);
    sidebarLayout->addWidget(m_sidebar);

    auto* btnAddFav = new QPushButton("收藏当前路径");
    btnAddFav->setFixedHeight(32);
    btnAddFav->setCursor(Qt::PointingHandCursor);
    btnAddFav->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #444; color: #AAA; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #3E3E42; color: #FFF; border-color: #666; }"
    );
    connect(btnAddFav, &QPushButton::clicked, this, [this](){
        QString p = m_pathInput->text().trimmed();
        if (QDir(p).exists()) addFavorite(p);
    });
    sidebarLayout->addWidget(btnAddFav);

    splitter->addWidget(sidebarWidget);

    // --- 中间主区域 (搜索功能) ---
    auto* centerWidget = new QWidget();
    auto* layout = new QVBoxLayout(centerWidget);
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setSpacing(10);

    auto* pathLayout = new QHBoxLayout();
    m_pathInput = new QLineEdit();
    m_pathInput->setPlaceholderText("双击查看历史，或在此粘贴路径...");
    m_pathInput->setClearButtonEnabled(true);
    m_pathInput->installEventFilter(this);
    connect(m_pathInput, &QLineEdit::returnPressed, this, &FileSearchWidget::onPathReturnPressed);
    
    auto* btnScan = new QToolButton();
    btnScan->setIcon(IconHelper::getIcon("scan", "#1abc9c", 18));
    btnScan->setToolTip(StringUtils::wrapToolTip("开始扫描"));
    btnScan->setFixedSize(38, 38);
    btnScan->setCursor(Qt::PointingHandCursor);
    btnScan->setStyleSheet("QToolButton { border: 1px solid #444; background: #2D2D30; border-radius: 6px; }"
                           "QToolButton:hover { background-color: #3E3E42; border-color: #007ACC; }");
    connect(btnScan, &QToolButton::clicked, this, &FileSearchWidget::onPathReturnPressed);

    auto* btnBrowse = new QToolButton();
    btnBrowse->setObjectName("ActionBtn");
    btnBrowse->setIcon(IconHelper::getIcon("folder", "#ffffff", 18));
    btnBrowse->setToolTip(StringUtils::wrapToolTip("浏览文件夹"));
    btnBrowse->setFixedSize(38, 38);
    btnBrowse->setCursor(Qt::PointingHandCursor);
    connect(btnBrowse, &QToolButton::clicked, this, &FileSearchWidget::selectFolder);

    pathLayout->addWidget(m_pathInput);
    pathLayout->addWidget(btnScan);
    pathLayout->addWidget(btnBrowse);
    layout->addLayout(pathLayout);

    auto* searchLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("输入文件名过滤...");
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->installEventFilter(this);
    connect(m_searchInput, &QLineEdit::textChanged, this, &FileSearchWidget::refreshList);
    connect(m_searchInput, &QLineEdit::returnPressed, this, [this](){
        addSearchHistoryEntry(m_searchInput->text().trimmed());
    });

    m_extInput = new QLineEdit();
    m_extInput->setPlaceholderText("后缀 (如 py)");
    m_extInput->setClearButtonEnabled(true);
    m_extInput->setFixedWidth(120);
    m_extInput->installEventFilter(this);
    connect(m_extInput, &QLineEdit::textChanged, this, &FileSearchWidget::refreshList);
    connect(m_extInput, &QLineEdit::returnPressed, this, [this](){
        addExtHistoryEntry(m_extInput->text().trimmed());
    });

    searchLayout->addWidget(m_searchInput);
    searchLayout->addWidget(m_extInput);
    layout->addLayout(searchLayout);

    auto* infoLayout = new QHBoxLayout();
    m_infoLabel = new QLabel("等待操作...");
    m_infoLabel->setStyleSheet("color: #888888; font-size: 12px;");
    
    m_showHiddenCheck = new QCheckBox("显示隐性文件");
    m_showHiddenCheck->setStyleSheet(R"(
        QCheckBox { color: #888; font-size: 12px; spacing: 5px; }
        QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #444; border-radius: 3px; background: #2D2D30; }
        QCheckBox::indicator:checked { background-color: #007ACC; border-color: #007ACC; }
        QCheckBox::indicator:hover { border-color: #666; }
    )");
    connect(m_showHiddenCheck, &QCheckBox::toggled, this, &FileSearchWidget::refreshList);

    infoLayout->addWidget(m_infoLabel);
    infoLayout->addWidget(m_showHiddenCheck);
    infoLayout->addStretch();
    layout->addLayout(infoLayout);

    auto* listHeaderLayout = new QHBoxLayout();
    listHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto* listTitle = new QLabel("搜索结果");
    listTitle->setStyleSheet("color: #888; font-size: 11px; font-weight: bold; border: none; background: transparent;");
    
    auto* btnCopyAll = new QToolButton();
    btnCopyAll->setIcon(IconHelper::getIcon("copy", "#1abc9c", 14));
    btnCopyAll->setToolTip(StringUtils::wrapToolTip("复制全部搜索结果的路径"));
    btnCopyAll->setFixedSize(20, 20);
    btnCopyAll->setCursor(Qt::PointingHandCursor);
    btnCopyAll->setStyleSheet("QToolButton { border: none; background: transparent; padding: 2px; }"
                               "QToolButton:hover { background-color: #3E3E42; border-radius: 4px; }");
    connect(btnCopyAll, &QToolButton::clicked, this, [this](){
        if (m_fileList->count() == 0) {
            QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 结果列表为空</b>"), this, {}, 2000);
            return;
        }
        QStringList paths;
        for (int i = 0; i < m_fileList->count(); ++i) {
            QString p = m_fileList->item(i)->data(Qt::UserRole).toString();
            if (!p.isEmpty()) paths << p;
        }
        if (paths.isEmpty()) return;
        QApplication::clipboard()->setText(paths.join("\n"));
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#2ecc71;'>✔ 已复制全部搜索结果</b>"), this, {}, 2000);
    });

    listHeaderLayout->addWidget(listTitle);
    listHeaderLayout->addStretch();
    listHeaderLayout->addWidget(btnCopyAll);
    layout->addLayout(listHeaderLayout);

    m_fileList = new QListWidget();
    m_fileList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fileList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileList, &QListWidget::customContextMenuRequested, this, &FileSearchWidget::showFileContextMenu);
    
    auto* actionSelectAll = new QAction(this);
    actionSelectAll->setShortcut(QKeySequence("Ctrl+A"));
    actionSelectAll->setShortcutContext(Qt::WidgetShortcut);
    connect(actionSelectAll, &QAction::triggered, [this](){ m_fileList->selectAll(); });
    m_fileList->addAction(actionSelectAll);

    auto* actionCopy = new QAction(this);
    actionCopy->setShortcut(QKeySequence("Ctrl+C"));
    actionCopy->setShortcutContext(Qt::WidgetShortcut);
    connect(actionCopy, &QAction::triggered, this, [this](){ copySelectedFiles(); });
    m_fileList->addAction(actionCopy);

    auto* actionDelete = new QAction(this);
    actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(actionDelete, &QAction::triggered, this, [this](){ onDeleteFile(); });
    m_fileList->addAction(actionDelete);

    layout->addWidget(m_fileList);

    splitter->addWidget(centerWidget);

    // --- 右侧边栏 (文件收藏) ---
    auto* rightSidebarWidget = new QWidget();
    auto* rightSidebarLayout = new QVBoxLayout(rightSidebarWidget);
    rightSidebarLayout->setContentsMargins(5, 0, 0, 0);
    rightSidebarLayout->setSpacing(10);

    auto* rightHeaderLayout = new QHBoxLayout();
    rightHeaderLayout->setSpacing(5);
    auto* rightSidebarIcon = new QLabel();
    rightSidebarIcon->setPixmap(IconHelper::getIcon("star", "#888").pixmap(14, 14));
    rightSidebarIcon->setStyleSheet("border: none; background: transparent;");
    rightHeaderLayout->addWidget(rightSidebarIcon);

    auto* rightSidebarHeader = new QLabel("文件收藏");
    rightSidebarHeader->setStyleSheet("color: #888; font-weight: bold; font-size: 12px; border: none; background: transparent;");
    rightHeaderLayout->addWidget(rightSidebarHeader);
    rightHeaderLayout->addStretch();
    rightSidebarLayout->addLayout(rightHeaderLayout);

    m_fileFavoritesList = new QListWidget();
    m_fileFavoritesList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fileFavoritesList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fileFavoritesList->setMinimumWidth(200);
    m_fileFavoritesList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileFavoritesList, &QListWidget::customContextMenuRequested, this, &FileSearchWidget::showFileFavoriteContextMenu);
    connect(m_fileFavoritesList, &QListWidget::itemDoubleClicked, this, [](QListWidgetItem* item){
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    rightSidebarLayout->addWidget(m_fileFavoritesList);

    splitter->addWidget(rightSidebarWidget);

    splitter->setStretchFactor(0, 0); // 左
    splitter->setStretchFactor(1, 1); // 中
    splitter->setStretchFactor(2, 0); // 右
}

void FileSearchWidget::selectFolder() {
    QString d = QFileDialog::getExistingDirectory(this, "选择文件夹");
    if (!d.isEmpty()) {
        m_pathInput->setText(d);
        startScan(d);
    }
}

void FileSearchWidget::onPathReturnPressed() {
    QString p = m_pathInput->text().trimmed();
    if (QDir(p).exists()) {
        startScan(p);
    } else {
        m_infoLabel->setText("路径不存在");
        m_pathInput->setStyleSheet("border: 1px solid #FF3333;");
    }
}

void FileSearchWidget::startScan(const QString& path) {
    m_pathInput->setStyleSheet("");
    
    // 开始扫描时自动保存搜索词和后缀的历史
    QString searchTxt = m_searchInput->text().trimmed();
    if (!searchTxt.isEmpty()) addSearchHistoryEntry(searchTxt);
    
    QString extTxt = m_extInput->text().trimmed();
    if (!extTxt.isEmpty()) addExtHistoryEntry(extTxt);

    if (m_scanThread) {
        m_scanThread->stop();
        m_scanThread->deleteLater();
    }

    m_fileList->clear();
    m_filesData.clear();
    m_visibleCount = 0;
    m_hiddenCount = 0;
    m_infoLabel->setText("正在扫描: " + path);

    m_scanThread = new ScannerThread(path, this);
    connect(m_scanThread, &ScannerThread::fileFound, this, &FileSearchWidget::onFileFound);
    connect(m_scanThread, &ScannerThread::finished, this, &FileSearchWidget::onScanFinished);
    m_scanThread->start();
}

void FileSearchWidget::onFileFound(const QString& name, const QString& path, bool isHidden) {
    m_filesData.append({name, path, isHidden});
    if (isHidden) m_hiddenCount++;
    else m_visibleCount++;

    if (m_filesData.size() % 300 == 0) {
        m_infoLabel->setText(QString("已发现 %1 个文件 (可见:%2 隐性:%3)...").arg(m_filesData.size()).arg(m_visibleCount).arg(m_hiddenCount));
    }
}

void FileSearchWidget::onScanFinished(int count) {
    m_infoLabel->setText(QString("扫描结束，共 %1 个文件 (可见:%2 隐性:%3)").arg(count).arg(m_visibleCount).arg(m_hiddenCount));
    addHistoryEntry(m_pathInput->text().trimmed());
    
    std::sort(m_filesData.begin(), m_filesData.end(), [](const FileData& a, const FileData& b){
        return a.name.localeAwareCompare(b.name) < 0;
    });

    refreshList();
}

void FileSearchWidget::refreshList() {
    m_fileList->clear();
    QString fullTxt = m_searchInput->text().toLower();
    QStringList keywords = fullTxt.split(QRegularExpression("[,，]+"), Qt::SkipEmptyParts);
    
    QString ext = m_extInput->text().toLower().trimmed();
    if (ext.startsWith(".")) ext = ext.mid(1);

    bool showHidden = m_showHiddenCheck->isChecked();

    int limit = 500;
    int shown = 0;

    for (const auto& data : std::as_const(m_filesData)) {
        if (!showHidden && data.isHidden) continue;
        if (!ext.isEmpty() && !data.name.toLower().endsWith("." + ext)) continue;
        
        if (!keywords.isEmpty()) {
            bool found = false;
            for (const QString& kw : keywords) {
                if (data.name.toLower().contains(kw.trimmed())) {
                    found = true;
                    break;
                }
            }
            if (!found) continue;
        }

        auto* item = new QListWidgetItem(data.name);
        item->setData(Qt::UserRole, data.path);
        item->setToolTip(StringUtils::wrapToolTip(data.path));
        m_fileList->addItem(item);
        
        shown++;
        if (shown >= limit) {
            auto* warn = new QListWidgetItem("--- 结果过多，仅显示前 500 条 ---");
            warn->setForeground(QColor(255, 170, 0));
            warn->setTextAlignment(Qt::AlignCenter);
            warn->setFlags(Qt::NoItemFlags);
            m_fileList->addItem(warn);
            break;
        }
    }
}

void FileSearchWidget::showFileContextMenu(const QPoint& pos) {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        auto* item = m_fileList->itemAt(pos);
        if (item) {
            item->setSelected(true);
            selectedItems << item;
        }
    }

    if (selectedItems.isEmpty()) return;

    QStringList paths;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) paths << p;
    }

    if (paths.isEmpty()) return;

    QMenu menu(this);
    menu.setWindowFlags(menu.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu.setAttribute(Qt::WA_TranslucentBackground);
    
    if (selectedItems.size() == 1) {
        QString filePath = paths.first();
        menu.addAction(IconHelper::getIcon("folder", "#F1C40F"), "定位文件夹", [filePath](){
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
        });
        menu.addAction(IconHelper::getIcon("search", "#4A90E2"), "定位文件", [filePath](){
#ifdef Q_OS_WIN
            QStringList args;
            args << "/select," << QDir::toNativeSeparators(filePath);
            QProcess::startDetached("explorer.exe", args);
#endif
        });
        menu.addAction(IconHelper::getIcon("edit", "#3498DB"), "编辑", [this](){ onEditFile(); });
        menu.addSeparator();
    }

    QString copyPathText = selectedItems.size() > 1 ? "复制选中路径" : "复制完整路径";
    menu.addAction(IconHelper::getIcon("copy", "#2ECC71"), copyPathText, [paths](){
        QApplication::clipboard()->setText(paths.join("\n"));
    });

    QString copyFileText = selectedItems.size() > 1 ? "复制选中文件" : "复制文件";
    menu.addAction(IconHelper::getIcon("file", "#4A90E2"), copyFileText, [this](){ copySelectedFiles(); });

    menu.addAction(IconHelper::getIcon("star", "#F1C40F"), "收藏文件", this, &FileSearchWidget::onFavoriteFile);

    menu.addAction(IconHelper::getIcon("merge", "#3498DB"), "合并选中内容", [this](){ onMergeSelectedFiles(); });

    menu.addSeparator();
    menu.addAction(IconHelper::getIcon("cut", "#E67E22"), "剪切", [this](){ onCutFile(); });
    menu.addAction(IconHelper::getIcon("trash", "#E74C3C"), "删除", [this](){ onDeleteFile(); });

    menu.exec(m_fileList->mapToGlobal(pos));
}

void FileSearchWidget::onEditFile() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    QStringList paths;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) paths << p;
    }
    if (paths.isEmpty()) return;

    QSettings settings("SearchTool", "ExternalEditor");
    QString editorPath = settings.value("EditorPath").toString();

    if (editorPath.isEmpty() || !QFile::exists(editorPath)) {
        QStringList commonPaths = {
            "C:/Program Files/Notepad++/notepad++.exe",
            "C:/Program Files (x86)/Notepad++/notepad++.exe"
        };
        for (const QString& p : commonPaths) {
            if (QFile::exists(p)) {
                editorPath = p;
                break;
            }
        }
    }

    if (editorPath.isEmpty() || !QFile::exists(editorPath)) {
        editorPath = QFileDialog::getOpenFileName(this, "选择编辑器 (推荐 Notepad++)", "C:/Program Files", "Executable (*.exe)");
        if (editorPath.isEmpty()) return;
        settings.setValue("EditorPath", editorPath);
    }

    for (const QString& filePath : paths) {
        QProcess::startDetached(editorPath, { QDir::toNativeSeparators(filePath) });
    }
}

void FileSearchWidget::copySelectedFiles() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    QList<QUrl> urls;
    QStringList paths;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) {
            urls << QUrl::fromLocalFile(p);
            paths << p;
        }
    }
    if (urls.isEmpty()) return;

    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(urls);
    mimeData->setText(paths.join("\n"));

    QApplication::clipboard()->setMimeData(mimeData);

    QString msg = selectedItems.size() > 1 ? QString("✔ 已复制 %1 个文件").arg(selectedItems.size()) : "✔ 已复制到剪贴板";
    QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>%1</b>").arg(msg)), this);
}

void FileSearchWidget::onCutFile() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    QList<QUrl> urls;
    for (auto* item : std::as_const(selectedItems)) {
        QString p = item->data(Qt::UserRole).toString();
        if (!p.isEmpty()) urls << QUrl::fromLocalFile(p);
    }
    if (urls.isEmpty()) return;

    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(urls);
    
#ifdef Q_OS_WIN
    QByteArray data;
    data.resize(4);
    data[0] = 2; // DROPEFFECT_MOVE
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    mimeData->setData("Preferred DropEffect", data);
#endif

    QApplication::clipboard()->setMimeData(mimeData);

    QString msg = selectedItems.size() > 1 ? QString("✔ 已剪切 %1 个文件").arg(selectedItems.size()) : "✔ 已剪切到剪贴板";
    QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>%1</b>").arg(msg)), this);
}

void FileSearchWidget::onDeleteFile() {
    auto selectedItems = m_fileList->selectedItems();
    if (selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 请先选择要操作的内容</b>"), this, {}, 2000);
        return;
    }

    int successCount = 0;
    for (auto* item : std::as_const(selectedItems)) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (filePath.isEmpty()) continue;

        if (QFile::moveToTrash(filePath)) {
            successCount++;
            for (int i = 0; i < m_filesData.size(); ++i) {
                if (m_filesData[i].path == filePath) {
                    m_filesData.removeAt(i);
                    break;
                }
            }
            delete item; 
        }
    }

    if (successCount > 0) {
        QString msg = selectedItems.size() > 1 ? QString("✔ %1 个文件已移至回收站").arg(successCount) : "✔ 文件已移至回收站";
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>%1</b>").arg(msg)), this);
        m_infoLabel->setText(msg);
    } else if (!selectedItems.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color: #e74c3c;'>✖ 无法删除文件，请检查是否被占用</b>"), this);
    }
}

void FileSearchWidget::onMergeFiles(const QStringList& filePaths, const QString& rootPath) {
    if (filePaths.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 没有可合并的文件</b>"), this, {}, 2000);
        return;
    }

    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString outName = QString("%1_code_export.md").arg(ts);
    QString outPath = QDir(rootPath).filePath(outName);

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color:#e74c3c;'>✖ 无法创建输出文件</b>"), this, {}, 2000);
        return;
    }

    QTextStream out(&outFile);
    out.setEncoding(QStringConverter::Utf8);

    out << "# 代码导出结果 - " << ts << "\n\n";
    out << "**项目路径**: `" << rootPath << "`\n\n";
    out << "**文件总数**: " << filePaths.size() << "\n\n";

    QMap<QString, int> fileStats;
    for (const QString& fp : filePaths) {
        QString lang = getFileLanguage(fp);
        fileStats[lang]++;
    }

    out << "## 文件类型统计\n\n";
    QStringList langs = fileStats.keys();
    std::sort(langs.begin(), langs.end(), [&](const QString& a, const QString& b){
        return fileStats.value(a) > fileStats.value(b);
    });
    for (const QString& lang : std::as_const(langs)) {
        out << "- **" << lang << "**: " << fileStats.value(lang) << " 个文件\n";
    }
    out << "\n---\n\n";

    for (const QString& fp : filePaths) {
        QString relPath = QDir(rootPath).relativeFilePath(fp);
        QString lang = getFileLanguage(fp);

        out << "
```

## 文件: `src/windows/FileSearchWindow.h`

```cpp
#ifndef FILESEARCHWINDOW_H
#define FILESEARCHWINDOW_H

#include "FramelessDialog.h"
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QThread>
#include <QPair>
#include <QSplitter>
#include <QLabel>
#include <atomic>

class FileSearchHistoryPopup;

/**
 * @brief 扫描线程：实现增量扫描与目录剪枝
 */
class ScannerThread : public QThread {
    Q_OBJECT
public:
    explicit ScannerThread(const QString& folderPath, QObject* parent = nullptr);
    void stop();

signals:
    void fileFound(const QString& name, const QString& path, bool isHidden);
    void finished(int count);

protected:
    void run() override;

private:
    QString m_folderPath;
    std::atomic<bool> m_isRunning{true};
};

/**
 * @brief 文件查找核心部件
 */
class FileSearchWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileSearchWidget(QWidget* parent = nullptr);
    ~FileSearchWidget();

    // 历史记录操作接口
    void addHistoryEntry(const QString& path);
    QStringList getHistory() const;
    void clearHistory();
    void removeHistoryEntry(const QString& path);
    void useHistoryPath(const QString& path);

    // 文件名搜索历史相关
    void addSearchHistoryEntry(const QString& text);
    QStringList getSearchHistory() const;
    void removeSearchHistoryEntry(const QString& text);
    void clearSearchHistory();

    // 后缀名搜索历史相关
    void addExtHistoryEntry(const QString& text);
    QStringList getExtHistory() const;
    void removeExtHistoryEntry(const QString& text);
    void clearExtHistory();

private slots:
    void selectFolder();
    void onFavoriteFile();
    void removeFileFavorite();
    void showFileFavoriteContextMenu(const QPoint& pos);
    void onPathReturnPressed();
    void startScan(const QString& path);
    void onFileFound(const QString& name, const QString& path, bool isHidden);
    void onScanFinished(int count);
    void refreshList();
    void showFileContextMenu(const QPoint& pos);
    void copySelectedFiles();
    void onEditFile();
    void onCutFile();
    void onDeleteFile();
    void onMergeSelectedFiles();
    void onMergeFolderContent();
    
    // 侧边栏相关
    void onSidebarItemClicked(QListWidgetItem* item);
    void showSidebarContextMenu(const QPoint& pos);
    void addFavorite(const QString& path, bool pinned = false);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void initUI();
    void setupStyles();
    void loadFavorites();
    void saveFavorites();
    void loadFileFavorites();
    void saveFileFavorites();
    void refreshFileFavoritesList(const QString& filterPath = QString());
    void onMergeFiles(const QStringList& filePaths, const QString& rootPath);

    QListWidget* m_sidebar;
    QListWidget* m_fileFavoritesList;
    QLineEdit* m_pathInput;
    QLineEdit* m_searchInput;
    QLineEdit* m_extInput;
    QLabel* m_infoLabel;
    QCheckBox* m_showHiddenCheck;
    QListWidget* m_fileList;
    
    ScannerThread* m_scanThread = nullptr;
    FileSearchHistoryPopup* m_historyPopup = nullptr;
    
    struct FileData {
        QString name;
        QString path;
        bool isHidden;
    };
    QList<FileData> m_filesData;
    int m_visibleCount = 0;
    int m_hiddenCount = 0;
};

/**
 * @brief 文件查找窗口：封装了 FileSearchWidget
 */
class FileSearchWindow : public FramelessDialog {
    Q_OBJECT
public:
    explicit FileSearchWindow(QWidget* parent = nullptr);
    ~FileSearchWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    FileSearchWidget* m_searchWidget;
};

#endif // FILESEARCHWINDOW_H
```

## 文件: `src/ui/FloatingBall.cpp`

```cpp
#include "FloatingBall.h"
#include "../core/DatabaseManager.h"
#include "IconHelper.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainterPath>
#include <QtMath>
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QSettings>
#include <QApplication>
#include <utility>

FloatingBall::FloatingBall(QWidget* parent) 
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint) 
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAcceptDrops(true);
    setFixedSize(120, 120); // 1:1 复刻 Python 版尺寸
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FloatingBall::updatePhysics);
    m_timer->start(16);

    restorePosition();
    
    QSettings settings("SearchTool", "FloatingBall");
    QString savedSkin = settings.value("skin", "mocha").toString();
    switchSkin(savedSkin);
}

void FloatingBall::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    float cx = width() / 2.0f;
    float cy = height() / 2.0f;

    // 1. 绘制柔和投影 (根据皮肤形状动态适配，带羽化效果)
    painter.save();
    float s = 1.0f - (m_bookY / 25.0f); // 随高度缩放
    float shadowOpacity = 40 * s;
    
    if (m_skinName == "open") {
        // 摊开手稿皮肤：较宽的柔和投影
        float sw = 84, sh = 20;
        QRadialGradient grad(cx, cy + 35, sw/2);
        grad.setColorAt(0, QColor(0, 0, 0, shadowOpacity));
        grad.setColorAt(0.8, QColor(0, 0, 0, shadowOpacity * 0.3));
        grad.setColorAt(1, Qt::transparent);
        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QRectF(cx - (sw/2)*s, cy + 30, sw*s, sh*s));
    } else {
        // 笔记本皮肤：窄长且极度羽化的投影
        float sw = 48, sh = 12;
        QRadialGradient grad(cx, cy + 42, sw/2);
        grad.setColorAt(0, QColor(0, 0, 0, shadowOpacity));
        grad.setColorAt(0.7, QColor(0, 0, 0, shadowOpacity * 0.4));
        grad.setColorAt(1, Qt::transparent);
        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        // 严格限制宽度在本体(56px)以内，杜绝边缘露头
        painter.drawEllipse(QRectF(cx - (sw/2)*s, cy + 38, sw*s, sh*s));
    }
    painter.restore();

    // 2. 绘制粒子
    for (const auto& p : m_particles) {
        QColor c = p.color;
        c.setAlphaF(p.life);
        painter.setBrush(c);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(p.pos, p.size, p.size);
    }

    // 3. 绘制笔记本
    painter.save();
    painter.translate(cx, cy + m_bookY);
    renderBook(&painter, m_skinName, 0); // 在 paintEvent 中只有 y 偏移是 translate 处理的，book 内部绘制居中
    painter.restore();

    // 4. 绘制钢笔
    painter.save();
    // paintEvent 中 pen 的位置偏移已经在 translate 中处理了
    // 但是原始代码是 translate(cx + m_penX, cy + m_penY - 5);
    // renderPen 需要 relative 坐标吗？
    // 让我们保持 renderPen 只负责画笔本身，坐标变换在外部做。
    painter.translate(cx + m_penX, cy + m_penY - 5);
    painter.rotate(m_penAngle);
    renderPen(&painter, m_skinName, 0, 0, 0); // 坐标和旋转已在外部 Transform 中完成
    painter.restore();
}

void FloatingBall::renderBook(QPainter* p, const QString& skinName, float /*bookY*/) {
    // bookY 参数在此场景下其实不需要，因为 painter 已经 translate 了
    // 为了保持静态函数的通用性，我们保留接口
    
    p->setPen(Qt::NoPen);
    if (skinName == "open") {
        float w = 80, h = 50;
        p->rotate(-5);
        QPainterPath path;
        path.moveTo(-w/2, -h/2); path.lineTo(0, -h/2 + 4);
        path.lineTo(w/2, -h/2); path.lineTo(w/2, h/2);
        path.lineTo(0, h/2 + 4); path.lineTo(-w/2, h/2); path.closeSubpath();
        p->setBrush(QColor("#f8f8f5"));
        p->drawPath(path);
        // 中缝阴影
        QLinearGradient grad(-10, 0, 10, 0);
        grad.setColorAt(0, QColor(0,0,0,0)); grad.setColorAt(0.5, QColor(0,0,0,20)); grad.setColorAt(1, QColor(0,0,0,0));
        p->setBrush(grad);
        p->drawRect(QRectF(-5, -h/2+4, 10, h-4));
        // 横线
        p->setPen(QPen(QColor(200, 200, 200), 1));
        for (int y = (int)(-h/2)+15; y < (int)(h/2); y += 7) {
            p->drawLine(int(-w/2+5), y, -5, y+2);
            p->drawLine(5, y+2, int(w/2-5), y);
        }
    } else {
        float w = 56, h = 76;
        if (skinName == "classic") {
            p->setBrush(QColor("#ebebe6"));
            p->drawRoundedRect(QRectF(-w/2+6, -h/2+6, w, h), 3, 3);
            QLinearGradient grad(-w, -h, w, h);
            grad.setColorAt(0, QColor("#3c3c41")); grad.setColorAt(1, QColor("#141419"));
            p->setBrush(grad);
            p->drawRoundedRect(QRectF(-w/2, -h/2, w, h), 3, 3);
            p->setBrush(QColor(10, 10, 10, 200));
            p->drawRect(QRectF(w/2 - 12, -h/2, 6, h));
        } else if (skinName == "royal") {
            p->setBrush(QColor("#f0f0eb"));
            p->drawRoundedRect(QRectF(-w/2+6, -h/2+6, w, h), 2, 2);
            QLinearGradient grad(-w, -h, w, 0);
            grad.setColorAt(0, QColor("#282864")); grad.setColorAt(1, QColor("#0a0a32"));
            p->setBrush(grad);
            p->drawRoundedRect(QRectF(-w/2, -h/2, w, h), 2, 2);
            p->setBrush(QColor(218, 165, 32));
            float c_size = 12;
            QPolygonF poly; poly << QPointF(w/2, -h/2) << QPointF(w/2-c_size, -h/2) << QPointF(w/2, -h/2+c_size);
            p->drawPolygon(poly);
        } else if (skinName == "matcha") {
            p->setBrush(QColor("#fafaf5"));
            p->drawRoundedRect(QRectF(-w/2+5, -h/2+5, w, h), 3, 3);
            QLinearGradient grad(-w, -h, w, h);
            grad.setColorAt(0, QColor("#a0be96")); grad.setColorAt(1, QColor("#64825a"));
            p->setBrush(grad);
            p->drawRoundedRect(QRectF(-w/2, -h/2, w, h), 3, 3);
            p->setBrush(QColor(255, 255, 255, 200));
            p->drawRoundedRect(QRectF(-w/2+10, -20, 34, 15), 2, 2);
        } else { // mocha / default
            p->setBrush(QColor("#f5f0e1"));
            p->drawRoundedRect(QRectF(-w/2+6, -h/2+6, w, h), 3, 3);
            QLinearGradient grad(-w, -h, w, h);
            grad.setColorAt(0, QColor("#5a3c32")); grad.setColorAt(1, QColor("#321e19"));
            p->setBrush(grad);
            p->drawRoundedRect(QRectF(-w/2, -h/2, w, h), 3, 3);
            p->setBrush(QColor(120, 20, 30));
            p->drawRect(QRectF(w/2 - 15, -h/2, 8, h));
        }
    }
}

void FloatingBall::renderPen(QPainter* p, const QString& skinName, float, float, float) {
    p->setPen(Qt::NoPen);
    QColor c_light, c_mid, c_dark;
    if (skinName == "royal") {
        c_light = QColor(60, 60, 70); c_mid = QColor(20, 20, 25); c_dark = QColor(26, 26, 26);
    } else if (skinName == "classic") {
        c_light = QColor(80, 80, 80); c_mid = QColor(30, 30, 30); c_dark = QColor(10, 10, 10);
    } else if (skinName == "matcha") {
        c_light = QColor(255, 255, 250); c_mid = QColor(240, 240, 230); c_dark = QColor(200, 200, 190);
    } else {
        c_light = QColor(180, 60, 70); c_mid = QColor(140, 20, 30); c_dark = QColor(60, 5, 10);
    }

    QLinearGradient bodyGrad(-6, 0, 6, 0);
    bodyGrad.setColorAt(0.0, c_light); bodyGrad.setColorAt(0.5, c_mid); bodyGrad.setColorAt(1.0, c_dark);
    QPainterPath path_body; path_body.addRoundedRect(QRectF(-6, -23, 12, 46), 5, 5);
    p->setBrush(bodyGrad); p->drawPath(path_body);
    
    QPainterPath tipPath;
    tipPath.moveTo(-3, 23); tipPath.lineTo(3, 23); tipPath.lineTo(0, 37); tipPath.closeSubpath();
    QLinearGradient tipGrad(-5, 0, 5, 0);
    tipGrad.setColorAt(0, QColor(240, 230, 180)); tipGrad.setColorAt(1, QColor(190, 170, 100));
    p->setBrush(tipGrad); p->drawPath(tipPath);
    
    p->setBrush(QColor(220, 200, 140)); p->drawRect(QRectF(-6, 19, 12, 4));
    p->setBrush(QColor(210, 190, 130)); p->drawRoundedRect(QRectF(-1.5, -17, 3, 24), 1.5, 1.5);
}

void FloatingBall::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_pressPos = event->pos();
        m_isDragging = false; // 初始不进入拖拽，等待 move 判定
        m_penY += 3.0f; // 1:1 复刻 Python 按下弹性反馈
        update();
    }
}

void FloatingBall::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        if (!m_isDragging) {
            // 只有移动距离超过系统设定的拖拽阈值才开始移动
            if ((event->pos() - m_pressPos).manhattanLength() > QApplication::startDragDistance()) {
                m_isDragging = true;
                m_offset = m_pressPos;
            }
        }
        
        if (m_isDragging) {
            QPoint newPos = event->globalPosition().toPoint() - m_offset;
            QScreen* screen = QGuiApplication::screenAt(event->globalPosition().toPoint());
            if (!screen) screen = QGuiApplication::primaryScreen();
            
            if (screen) {
                QRect ag = screen->availableGeometry();
                int x = qBound(ag.left(), newPos.x(), ag.right() - width());
                int y = qBound(ag.top(), newPos.y(), ag.bottom() - height());
                newPos = QPoint(x, y);
            }
            move(newPos);
        }
    }
}

void FloatingBall::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        savePosition();
    }
}

void FloatingBall::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}

void FloatingBall::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    m_isHovering = true;
}

void FloatingBall::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    m_isHovering = false;
}

void FloatingBall::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #2D2D2D; color: #EEE; border: 1px solid #444; padding: 4px; } "
        /* 10px 间距规范：padding-left 10px + icon margin-left 6px */
        "QMenu::item { padding: 6px 10px 6px 10px; border-radius: 3px; } "
        "QMenu::icon { margin-left: 6px; } "
        "QMenu::item:selected { background-color: #4a90e2; color: white; } "
        "QMenu::separator { background-color: #444; height: 1px; margin: 4px 0; }"
    );

    QMenu* skinMenu = menu.addMenu(IconHelper::getIcon("palette", "#aaaaaa", 18), "切换外观");
    skinMenu->setStyleSheet(menu.styleSheet());
    skinMenu->addAction(IconHelper::getIcon("coffee", "#BCAAA4", 18), "摩卡·勃艮第", [this](){ switchSkin("mocha"); });
    skinMenu->addAction(IconHelper::getIcon("grid", "#90A4AE", 18), "经典黑金", [this](){ switchSkin("classic"); });
    skinMenu->addAction(IconHelper::getIcon("book", "#9FA8DA", 18), "皇家蓝", [this](){ switchSkin("royal"); });
    skinMenu->addAction(IconHelper::getIcon("leaf", "#A5D6A7", 18), "抹茶绿", [this](){ switchSkin("matcha"); });
    skinMenu->addAction(IconHelper::getIcon("book_open", "#FFCC80", 18), "摊开手稿", [this](){ switchSkin("open"); });
    skinMenu->addAction("默认天蓝", [this](){ switchSkin("default"); });

    menu.addSeparator();
    menu.addAction(IconHelper::getIcon("zap", "#aaaaaa", 18), "打开快速笔记", this, &FloatingBall::requestQuickWindow);
    menu.addAction(IconHelper::getIcon("monitor", "#aaaaaa", 18), "打开主界面", this, &FloatingBall::requestMainWindow);
    menu.addAction(IconHelper::getIcon("toolbox", "#aaaaaa", 18), "打开工具箱", this, &FloatingBall::requestToolbox);
    menu.addAction(IconHelper::getIcon("add", "#aaaaaa", 18), "新建灵感", this, &FloatingBall::requestNewIdea);
    menu.addSeparator();
    menu.addAction(IconHelper::getIcon("power", "#aaaaaa", 18), "退出程序", [](){ qApp->quit(); });
    
    menu.exec(event->globalPos());
}

void FloatingBall::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasText()) {
        event->accept();
        m_isHovering = true;
    } else {
        event->ignore();
    }
}

void FloatingBall::dragLeaveEvent(QDragLeaveEvent* event) {
    Q_UNUSED(event);
    m_isHovering = false;
}

void FloatingBall::dropEvent(QDropEvent* event) {
    m_isHovering = false;
    QString text = event->mimeData()->text();
    if (!text.trimmed().isEmpty()) {
        // 提取第一个非空行作为标题
        QString title;
        QStringList lines = text.split('\n');
        for (const QString& line : std::as_const(lines)) {
            QString trimmed = line.trimmed();
            if (!trimmed.isEmpty()) {
                title = trimmed.left(40);
                if (trimmed.length() > 40) title += "...";
                break;
            }
        }

        if (title.isEmpty()) {
            title = "拖拽创建数据";
        }

        DatabaseManager::instance().addNoteAsync(title, text, {"拖拽"}, "", -1, "text");
        burstParticles();
        m_isWriting = true;
        m_writeTimer = 0;
        event->acceptProposedAction();
    }
}

QIcon FloatingBall::generateBallIcon() {
    QPixmap pixmap(120, 120);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    float cx = 60.0f;
    float cy = 60.0f;
    
    // 静态状态参数 (无动画)
    float bookY = 0.0f;
    float penX = 0.0f;
    float penY = 0.0f;
    float penAngle = -45.0f;
    QString skinName = "mocha";
    
    // 柔和投影 (图标模式保持静态最佳效果)
    painter.save();
    float sw = 48, sh = 12;
    QRadialGradient grad(cx, cy + 42, sw/2);
    grad.setColorAt(0, QColor(0, 0, 0, 35));
    grad.setColorAt(0.7, QColor(0, 0, 0, 15));
    grad.setColorAt(1, Qt::transparent);
    painter.setBrush(grad);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QRectF(cx - sw/2, cy + 38, sw, sh));
    painter.restore();
    
    // 笔记本
    painter.save();
    painter.translate(cx, cy + bookY);
    renderBook(&painter, skinName, 0);
    painter.restore();
    
    // 钢笔
    painter.save();
    painter.translate(cx + penX, cy + penY - 5);
    painter.rotate(penAngle);
    renderPen(&painter, skinName, 0, 0, 0);
    painter.restore();
    
    return QIcon(pixmap);
}

void FloatingBall::switchSkin(const QString& name) {
    m_skinName = name;
    
    QSettings settings("SearchTool", "FloatingBall");
    settings.setValue("skin", name);
    
    update();
}

void FloatingBall::burstParticles() {
    // 逻辑保持
}

void FloatingBall::updatePhysics() {
    m_timeStep += 0.05f;
    
    // 1. 待机呼吸
    float idlePenY = qSin(m_timeStep * 0.5f) * 4.0f;
    float idleBookY = qSin(m_timeStep * 0.5f - 1.0f) * 2.0f;
    
    float targetPenAngle = -45.0f;
    float targetPenX = 0.0f;
    float targetPenY = idlePenY;
    float targetBookY = idleBookY;
    
    // 2. 书写/悬停动画
    if (m_isWriting || m_isHovering) {
        m_writeTimer++;
        targetPenAngle = -65.0f;
        float writeSpeed = m_timeStep * 3.0f;
        targetPenX = qSin(writeSpeed) * 8.0f;
        targetPenY = 5.0f + qCos(writeSpeed * 2.0f) * 2.0f;
        targetBookY = -3.0f;
        
        if (m_isWriting && m_writeTimer > 90) {
            m_isWriting = false;
        }
    }
    
    // 3. 物理平滑
    float easing = 0.1f;
    m_penAngle += (targetPenAngle - m_penAngle) * easing;
    m_penX += (targetPenX - m_penX) * easing;
    m_penY += (targetPenY - m_penY) * easing;
    m_bookY += (targetBookY - m_bookY) * easing;

    updateParticles();
    update();
}

void FloatingBall::updateParticles() {
    if ((m_isWriting || m_isHovering) && m_particles.size() < 15) {
        if (QRandomGenerator::global()->generateDouble() < 0.3) {
            float rad = qDegreesToRadians(m_penAngle);
            float tipLen = 35.0f;
            Particle p;
            p.pos = QPointF(width()/2.0f + m_penX - qSin(rad)*tipLen, height()/2.0f + m_penY + qCos(rad)*tipLen);
            p.velocity = QPointF(QRandomGenerator::global()->generateDouble() - 0.5, QRandomGenerator::global()->generateDouble() + 0.5);
            p.life = 1.0;
            p.size = 1.0f + QRandomGenerator::global()->generateDouble() * 2.0f;
            p.color = QColor::fromHsv(QRandomGenerator::global()->bounded(360), 150, 255);
            m_particles.append(p);
        }
    }
    for (int i = 0; i < m_particles.size(); ++i) {
        m_particles[i].pos += m_particles[i].velocity;
        m_particles[i].life -= 0.03;
        m_particles[i].size *= 0.96f;
        if (m_particles[i].life <= 0) {
            m_particles.removeAt(i);
            --i;
        }
    }
}

void FloatingBall::savePosition() {
    QSettings settings("SearchTool", "FloatingBall");
    settings.setValue("pos", pos());
    settings.setValue("visible", isVisible());
}

void FloatingBall::restorePosition() {
    QSettings settings("SearchTool", "FloatingBall");
    if (settings.value("visible", true).toBool()) {
        show();
    } else {
        hide();
    }

    if (settings.contains("pos")) {
        QPoint savedPos = settings.value("pos").toPoint();
        QScreen* screen = QGuiApplication::screenAt(savedPos);
        if (!screen) screen = QGuiApplication::primaryScreen();
        
        if (screen) {
            QRect ag = screen->availableGeometry();
            int x = qBound(ag.left(), savedPos.x(), ag.right() - width());
            int y = qBound(ag.top(), savedPos.y(), ag.bottom() - height());
            move(x, y);
        } else {
            move(savedPos);
        }
    } else {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect ag = screen->availableGeometry();
            move(ag.right() - 150, ag.top() + ag.height() / 2 - height() / 2);
        }
    }
}
```

## 文件: `src/ui/FloatingBall.h`

```cpp
#ifndef FLOATINGBALL_H
#define FLOATINGBALL_H

#include <QWidget>
#include <QPoint>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "WritingAnimation.h"

class FloatingBall : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move)

public:
    explicit FloatingBall(QWidget* parent = nullptr);
    static QIcon generateBallIcon();
    void savePosition();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    static void renderBook(QPainter* p, const QString& skinName, float bookY);
    static void renderPen(QPainter* p, const QString& skinName, float penX, float penY, float penAngle);

private:
    void switchSkin(const QString& name);
    // drawBook 和 drawPen 已改为静态 renderBook/renderPen
    void burstParticles();
    void updatePhysics();
    void updateParticles();
    void restorePosition();

    QPoint m_pressPos;
    QPoint m_offset;
    bool m_isDragging = false;
    bool m_isHovering = false;
    bool m_isWriting = false;
    int m_writeTimer = 0;

    QTimer* m_timer;
    float m_timeStep = 0.0f;
    float m_penX = 0.0f;
    float m_penY = 0.0f;
    float m_penAngle = -45.0f;
    float m_bookY = 0.0f;

    struct Particle {
        QPointF pos;
        QPointF velocity;
        double life;
        float size;
        QColor color;
    };
    QList<Particle> m_particles;

    QString m_skinName = "mocha";

signals:
    void doubleClicked();
    void requestMainWindow();
    void requestQuickWindow();
    void requestToolbox();
    void requestNewIdea();
};

#endif // FLOATINGBALL_H
```

## 文件: `src/windows/FramelessDialog.cpp`

```cpp
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
    QSettings settings("SearchTool", "WindowStates");
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
    QSettings settings("SearchTool", "WindowStates");
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
```

## 文件: `src/windows/FramelessDialog.h`

```cpp
#ifndef FRAMELESSDIALOG_H
#define FRAMELESSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>

class QGraphicsDropShadowEffect;

/**
 * @brief 无边框对话框基类，自带标题栏、关闭按钮、阴影、置顶
 */
class FramelessDialog : public QDialog {
    Q_OBJECT
public:
    explicit FramelessDialog(const QString& title, QWidget* parent = nullptr);
    virtual ~FramelessDialog() = default;

    void setStayOnTop(bool stay);

private slots:
    void toggleStayOnTop(bool checked);
    void toggleMaximize();

protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void leaveEvent(QEvent* event) override;

    QWidget* m_contentArea;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_outerLayout;
    QWidget* m_container;
    QGraphicsDropShadowEffect* m_shadow;
    QLabel* m_titleLabel;
    QPushButton* m_btnPin;
    QPushButton* m_minBtn;
    QPushButton* m_maxBtn;
    QPushButton* m_closeBtn;

    virtual void loadWindowSettings();
    virtual void saveWindowSettings();

private:
    enum ResizeEdge {
        None = 0,
        Top = 0x1,
        Bottom = 0x2,
        Left = 0x4,
        Right = 0x8,
        TopLeft = Top | Left,
        TopRight = Top | Right,
        BottomLeft = Bottom | Left,
        BottomRight = Bottom | Right
    };

    ResizeEdge getEdge(const QPoint& pos);
    void updateCursor(ResizeEdge edge);

    QPoint m_dragPos;
    bool m_isStayOnTop = false; 
    bool m_firstShow = true;
    bool m_isResizing = false;
    ResizeEdge m_resizeEdge = None;
    const int m_padding = 5; 
};

#endif // FRAMELESSDIALOG_H
```

## 文件: `src/utils/IconHelper.h`

```cpp
#ifndef ICONHELPER_H
#define ICONHELPER_H

#include <QIcon>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include "SvgIcons.h"

class IconHelper {
public:
    static QIcon getIcon(const QString& name, const QString& color = "#cccccc", int size = 64) {
        if (!SvgIcons::icons.contains(name)) return QIcon();

        QString svgData = SvgIcons::icons[name];
        svgData.replace("currentColor", color);
        // 如果 svg 中没有 currentColor，强制替换所有可能的 stroke/fill 颜色（简易实现）
        // 这里假设 SVG 字符串格式标准，仅替换 stroke="currentColor" 或 fill="currentColor"
        // 实际上 Python 版是直接全量 replace "currentColor"

        QByteArray bytes = svgData.toUtf8();
        QSvgRenderer renderer(bytes);
        
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);
        
        QIcon icon;
        icon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
        icon.addPixmap(pixmap, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pixmap, QIcon::Active, QIcon::On);
        icon.addPixmap(pixmap, QIcon::Active, QIcon::Off);
        icon.addPixmap(pixmap, QIcon::Selected, QIcon::On);
        icon.addPixmap(pixmap, QIcon::Selected, QIcon::Off);
        return icon;
    }
};

#endif // ICONHELPER_H
```

## 文件: `src/windows/KeywordSearchWindow.cpp`

```cpp
#include "KeywordSearchWindow.h"
#include "../utils/IconHelper.h"
#include "../utils/StringUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QDirIterator>
#include <utility>
#include <QTextStream>
#include <QRegularExpression>
#include <QDateTime>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QtConcurrent>
#include <QScrollBar>
#include <QToolTip>
#include <QSettings>
#include <QMenu>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextBrowser>

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

// ----------------------------------------------------------------------------
// Sidebar ListWidget subclass for Drag & Drop
// ----------------------------------------------------------------------------
class KeywordSidebarListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit KeywordSidebarListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setAcceptDrops(true);
    }
signals:
    void folderDropped(const QString& path);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
            event->acceptProposedAction();
        }
    }
    void dragMoveEvent(QDragMoveEvent* event) override {
        event->acceptProposedAction();
    }
    void dropEvent(QDropEvent* event) override {
        QString path;
        if (event->mimeData()->hasUrls()) {
            path = event->mimeData()->urls().at(0).toLocalFile();
        } else if (event->mimeData()->hasText()) {
            path = event->mimeData()->text();
        }
        
        if (!path.isEmpty() && QDir(path).exists()) {
            emit folderDropped(path);
            event->acceptProposedAction();
        }
    }
};

/**
 * @brief 自定义列表项，支持置顶排序逻辑
 */
class KeywordFavoriteItem : public QListWidgetItem {
public:
    using QListWidgetItem::QListWidgetItem;
    bool operator<(const QListWidgetItem &other) const override {
        bool thisPinned = data(Qt::UserRole + 1).toBool();
        bool otherPinned = other.data(Qt::UserRole + 1).toBool();
        if (thisPinned != otherPinned) return thisPinned; 
        return text().localeAwareCompare(other.text()) < 0;
    }
};

// ----------------------------------------------------------------------------
// KeywordSearchHistory 相关辅助类 (复刻 FileSearchHistoryPopup 逻辑)
// ----------------------------------------------------------------------------
class KeywordChip : public QFrame {
    Q_OBJECT
public:
    KeywordChip(const QString& text, QWidget* parent = nullptr) : QFrame(parent), m_text(text) {
        setAttribute(Qt::WA_StyledBackground);
        setCursor(Qt::PointingHandCursor);
        setObjectName("KeywordChip");
        
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 6, 10, 6);
        layout->setSpacing(10);
        
        auto* lbl = new QLabel(text);
        lbl->setStyleSheet("border: none; background: transparent; color: #DDD; font-size: 13px;");
        layout->addWidget(lbl);
        layout->addStretch();
        
        auto* btnDel = new QPushButton();
        btnDel->setIcon(IconHelper::getIcon("close", "#666", 16));
        btnDel->setIconSize(QSize(10, 10));
        btnDel->setFixedSize(16, 16);
        btnDel->setCursor(Qt::PointingHandCursor);
        btnDel->setStyleSheet(
            "QPushButton { background-color: transparent; border-radius: 4px; padding: 0px; }"
            "QPushButton:hover { background-color: #E74C3C; }"
        );
        
        connect(btnDel, &QPushButton::clicked, this, [this](){ emit deleted(m_text); });
        layout->addWidget(btnDel);

        setStyleSheet(
            "#KeywordChip { background-color: transparent; border: none; border-radius: 4px; }"
            "#KeywordChip:hover { background-color: #3E3E42; }"
        );
    }
    
    void mousePressEvent(QMouseEvent* e) override { 
        if(e->button() == Qt::LeftButton) emit clicked(m_text); 
        QFrame::mousePressEvent(e);
    }

signals:
    void clicked(const QString& text);
    void deleted(const QString& text);
private:
    QString m_text;
};

class KeywordSearchHistoryPopup : public QWidget {
    Q_OBJECT
public:
    enum Type { Path, Keyword, Replace };

    explicit KeywordSearchHistoryPopup(KeywordSearchWidget* widget, QLineEdit* edit, Type type) 
        : QWidget(widget->window(), Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) 
    {
        m_widget = widget;
        m_edit = edit;
        m_type = type;
        setAttribute(Qt::WA_TranslucentBackground);
        
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(12, 12, 12, 12);
        
        auto* container = new QFrame();
        container->setObjectName("PopupContainer");
        container->setStyleSheet(
            "#PopupContainer { background-color: #252526; border: 1px solid #444; border-radius: 10px; }"
        );
        rootLayout->addWidget(container);

        auto* shadow = new QGraphicsDropShadowEffect(container);
        shadow->setBlurRadius(20); shadow->setXOffset(0); shadow->setYOffset(5);
        shadow->setColor(QColor(0, 0, 0, 120));
        container->setGraphicsEffect(shadow);

        auto* layout = new QVBoxLayout(container);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(10);

        auto* top = new QHBoxLayout();
        QString titleStr = "最近记录";
        if (m_type == Path) titleStr = "最近扫描路径";
        else if (m_type == Keyword) titleStr = "最近查找内容";
        else if (m_type == Replace) titleStr = "最近替换内容";

        auto* icon = new QLabel();
        icon->setPixmap(IconHelper::getIcon("clock", "#888").pixmap(14, 14));
        icon->setStyleSheet("border: none; background: transparent;");
        icon->setToolTip(StringUtils::wrapToolTip(titleStr));
        top->addWidget(icon);

        top->addStretch();

        auto* clearBtn = new QPushButton();
        clearBtn->setIcon(IconHelper::getIcon("trash", "#666", 14));
        clearBtn->setIconSize(QSize(14, 14));
        clearBtn->setFixedSize(20, 20);
        clearBtn->setCursor(Qt::PointingHandCursor);
        clearBtn->setToolTip(StringUtils::wrapToolTip("清空历史记录"));
        clearBtn->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background-color: rgba(231, 76, 60, 0.2); }");
        connect(clearBtn, &QPushButton::clicked, [this](){
            clearAllHistory();
            refreshUI();
        });
        top->addWidget(clearBtn);
        layout->addLayout(top);

        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet(
            "QScrollArea { background-color: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background-color: transparent; }"
        );
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        m_chipsWidget = new QWidget();
        m_chipsWidget->setStyleSheet("background-color: transparent;");
        m_vLayout = new QVBoxLayout(m_chipsWidget);
        m_vLayout->setContentsMargins(0, 0, 0, 0);
        m_vLayout->setSpacing(2);
        m_vLayout->addStretch();
        scroll->setWidget(m_chipsWidget);
        layout->addWidget(scroll);

        m_opacityAnim = new QPropertyAnimation(this, "windowOpacity");
        m_opacityAnim->setDuration(200);
    }

    void clearAllHistory() {
        QString key = "keywordList";
        if (m_type == Path) key = "pathList";
        else if (m_type == Replace) key = "replaceList";

        QSettings settings("SearchTool", "KeywordSearchHistory");
        settings.setValue(key, QStringList());
    }

    void removeEntry(const QString& text) {
        QString key = "keywordList";
        if (m_type == Path) key = "pathList";
        else if (m_type == Replace) key = "replaceList";

        QSettings settings("SearchTool", "KeywordSearchHistory");
        QStringList history = settings.value(key).toStringList();
        history.removeAll(text);
        settings.setValue(key, history);
    }

    QStringList getHistory() const {
        QString key = "keywordList";
        if (m_type == Path) key = "pathList";
        else if (m_type == Replace) key = "replaceList";

        QSettings settings("SearchTool", "KeywordSearchHistory");
        return settings.value(key).toStringList();
    }

    void refreshUI() {
        QLayoutItem* item;
        while ((item = m_vLayout->takeAt(0))) {
            if(item->widget()) item->widget()->deleteLater();
            delete item;
        }
        m_vLayout->addStretch();
        
        QStringList history = getHistory();
        if(history.isEmpty()) {
            auto* lbl = new QLabel("暂无历史记录");
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet("color: #555; font-style: italic; margin: 20px; border: none;");
            m_vLayout->insertWidget(0, lbl);
        } else {
            for(const QString& val : history) {
                auto* chip = new KeywordChip(val);
                chip->setFixedHeight(32);
                connect(chip, &KeywordChip::clicked, this, [this](const QString& v){ 
                    m_edit->setText(v);
                    close(); 
                });
                connect(chip, &KeywordChip::deleted, this, [this](const QString& v){ 
                    removeEntry(v);
                    refreshUI(); 
                });
                m_vLayout->insertWidget(m_vLayout->count() - 1, chip);
            }
        }
        
        int targetWidth = m_edit->width();
        int contentHeight = qMin(410, (int)history.size() * 34 + 60);
        setFixedWidth(targetWidth + 24);
        resize(targetWidth + 24, contentHeight);
    }

    void showAnimated() {
        refreshUI();
        QPoint pos = m_edit->mapToGlobal(QPoint(0, m_edit->height()));
        move(pos.x() - 12, pos.y() - 7);
        setWindowOpacity(0);
        show();
        m_opacityAnim->setStartValue(0);
        m_opacityAnim->setEndValue(1);
        m_opacityAnim->start();
    }

private:
    KeywordSearchWidget* m_widget;
    QLineEdit* m_edit;
    Type m_type;
    QWidget* m_chipsWidget;
    QVBoxLayout* m_vLayout;
    QPropertyAnimation* m_opacityAnim;
};

// ----------------------------------------------------------------------------
// KeywordSearchWidget 实现
// ----------------------------------------------------------------------------
KeywordSearchWidget::KeywordSearchWidget(QWidget* parent) : QWidget(parent) {
    m_ignoreDirs = {".git", ".svn", ".idea", ".vscode", "__pycache__", "node_modules", "dist", "build", "venv"};
    setupStyles();
    initUI();
    loadFavorites();
}

KeywordSearchWidget::~KeywordSearchWidget() {
}

void KeywordSearchWidget::setupStyles() {
    setStyleSheet(R"(
        QWidget {
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 14px;
            color: #E0E0E0;
            outline: none;
        }
        QSplitter::handle {
            background-color: #333;
        }
        QListWidget {
            background-color: #252526; 
            border: 1px solid #333333;
            border-radius: 6px;
            padding: 4px;
        }
        QListWidget::item {
            height: 30px;
            padding-left: 8px;
            border-radius: 4px;
            color: #CCCCCC;
        }
        QListWidget::item:selected {
            background-color: #37373D;
            border-left: 3px solid #007ACC;
            color: #FFFFFF;
        }
        QListWidget::item:hover {
            background-color: #2A2D2E;
        }
        QLineEdit {
            background-color: #333333;
            border: 1px solid #444444;
            color: #FFFFFF;
            border-radius: 6px;
            padding: 8px;
            selection-background-color: #264F78;
        }
        QLineEdit:focus {
            border: 1px solid #007ACC;
            background-color: #2D2D2D;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
}

void KeywordSearchWidget::initUI() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);

    // --- 左侧边栏 ---
    auto* sidebarWidget = new QWidget();
    auto* sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(0, 0, 5, 0);
    sidebarLayout->setSpacing(10);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(5);
    auto* sidebarIcon = new QLabel();
    sidebarIcon->setPixmap(IconHelper::getIcon("folder", "#888").pixmap(14, 14));
    sidebarIcon->setStyleSheet("border: none; background: transparent;");
    headerLayout->addWidget(sidebarIcon);

    auto* sidebarHeader = new QLabel("搜索根目录 (可拖入)");
    sidebarHeader->setStyleSheet("color: #888; font-weight: bold; font-size: 12px; border: none; background: transparent;");
    headerLayout->addWidget(sidebarHeader);
    headerLayout->addStretch();
    sidebarLayout->addLayout(headerLayout);

    m_sidebar = new KeywordSidebarListWidget();
    m_sidebar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebar->setMinimumWidth(200);
    m_sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(static_cast<KeywordSidebarListWidget*>(m_sidebar), &KeywordSidebarListWidget::folderDropped, this, [this](const QString& path){ addFavorite(path); });
    connect(m_sidebar, &QListWidget::itemClicked, this, &KeywordSearchWidget::onSidebarItemClicked);
    connect(m_sidebar, &QListWidget::customContextMenuRequested, this, &KeywordSearchWidget::showSidebarContextMenu);
    sidebarLayout->addWidget(m_sidebar);

    auto* btnAddFav = new QPushButton("收藏当前路径");
    btnAddFav->setFixedHeight(32);
    btnAddFav->setCursor(Qt::PointingHandCursor);
    btnAddFav->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #444; color: #AAA; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #3E3E42; color: #FFF; border-color: #666; }"
    );
    connect(btnAddFav, &QPushButton::clicked, this, [this](){
        QString p = m_pathEdit->text().trimmed();
        if (QDir(p).exists()) addFavorite(p);
    });
    sidebarLayout->addWidget(btnAddFav);

    splitter->addWidget(sidebarWidget);

    // --- 右侧内容区域 ---
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(5, 0, 0, 0);
    rightLayout->setSpacing(15);

    // --- 配置区域 ---
    auto* configGroup = new QWidget();
    auto* configLayout = new QGridLayout(configGroup);
    configLayout->setContentsMargins(0, 0, 0, 0);
    configLayout->setHorizontalSpacing(10); 
    configLayout->setVerticalSpacing(10);
    configLayout->setColumnStretch(1, 1);
    configLayout->setColumnStretch(0, 0);
    configLayout->setColumnStretch(2, 0);

    auto createLabel = [](const QString& text) {
        auto* lbl = new QLabel(text);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        lbl->setStyleSheet("color: #AAA; font-weight: bold; border: none; background: transparent;");
        return lbl;
    };

    auto setEditStyle = [](QLineEdit* edit) {
        edit->setClearButtonEnabled(true);
        edit->setStyleSheet(
            "QLineEdit { background: #252526; border: 1px solid #333; border-radius: 4px; padding: 6px; color: #EEE; }"
            "QLineEdit:focus { border-color: #007ACC; }"
        );
    };

    // 1. 搜索目录
    configLayout->addWidget(createLabel("搜索目录:"), 0, 0);
    m_pathEdit = new ClickableLineEdit();
    m_pathEdit->setPlaceholderText("选择搜索根目录 (双击查看历史)...");
    setEditStyle(m_pathEdit);
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &KeywordSearchWidget::onSearch);
    connect(m_pathEdit, &ClickableLineEdit::doubleClicked, this, &KeywordSearchWidget::onShowHistory);
    configLayout->addWidget(m_pathEdit, 0, 1);

    auto* browseBtn = new QPushButton();
    browseBtn->setFixedSize(38, 32);
    browseBtn->setIcon(IconHelper::getIcon("folder", "#EEE", 18));
    browseBtn->setToolTip(StringUtils::wrapToolTip("浏览文件夹"));
    browseBtn->setAutoDefault(false);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setStyleSheet("QPushButton { background: #3E3E42; border: none; border-radius: 4px; } QPushButton:hover { background: #4E4E52; }");
    connect(browseBtn, &QPushButton::clicked, this, &KeywordSearchWidget::onBrowseFolder);
    configLayout->addWidget(browseBtn, 0, 2);

    // 2. 文件过滤
    configLayout->addWidget(createLabel("文件过滤:"), 1, 0);
    m_filterEdit = new QLineEdit();
    m_filterEdit->setPlaceholderText("例如: *.py, *.txt (留空则扫描所有文本文件)");
    setEditStyle(m_filterEdit);
    connect(m_filterEdit, &QLineEdit::returnPressed, this, &KeywordSearchWidget::onSearch);
    configLayout->addWidget(m_filterEdit, 1, 1, 1, 2);

    // 3. 查找内容
    configLayout->addWidget(createLabel("查找内容:"), 2, 0);
    m_searchEdit = new ClickableLineEdit();
    m_searchEdit->setPlaceholderText("输入要查找的内容 (双击查看历史)...");
    setEditStyle(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &KeywordSearchWidget::onSearch);
    connect(m_searchEdit, &ClickableLineEdit::doubleClicked, this, &KeywordSearchWidget::onShowHistory);
    configLayout->addWidget(m_searchEdit, 2, 1);

    // 4. 替换内容
    configLayout->addWidget(createLabel("替换内容:"), 3, 0);
    m_replaceEdit = new ClickableLineEdit();
    m_replaceEdit->setPlaceholderText("替换为 (双击查看历史)...");
    setEditStyle(m_replaceEdit);
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &KeywordSearchWidget::onSearch);
    connect(m_replaceEdit, &ClickableLineEdit::doubleClicked, this, &KeywordSearchWidget::onShowHistory);
    configLayout->addWidget(m_replaceEdit, 3, 1);

    // 交换按钮 (跨越查找和替换行)
    auto* swapBtn = new QPushButton();
    swapBtn->setFixedSize(32, 74); 
    swapBtn->setCursor(Qt::PointingHandCursor);
    swapBtn->setToolTip(StringUtils::wrapToolTip("交换查找与替换内容"));
    swapBtn->setIcon(IconHelper::getIcon("swap", "#AAA", 20));
    swapBtn->setAutoDefault(false);
    swapBtn->setStyleSheet("QPushButton { background: #3E3E42; border: none; border-radius: 4px; } QPushButton:hover { background: #4E4E52; }");
    connect(swapBtn, &QPushButton::clicked, this, &KeywordSearchWidget::onSwapSearchReplace);
    configLayout->addWidget(swapBtn, 2, 2, 2, 1);

    // 选项
    m_caseCheck = new QCheckBox("区分大小写");
    m_caseCheck->setStyleSheet("QCheckBox { color: #AAA; }");
    configLayout->addWidget(m_caseCheck, 4, 1, 1, 2);

    rightLayout->addWidget(configGroup);

    // --- 按钮区域 ---
    auto* btnLayout = new QHBoxLayout();
    auto* searchBtn = new QPushButton(" 智能搜索");
    searchBtn->setAutoDefault(false);
    searchBtn->setIcon(IconHelper::getIcon("find_keyword", "#FFF", 16));
    searchBtn->setStyleSheet("QPushButton { background: #007ACC; border: none; border-radius: 4px; padding: 8px 20px; color: #FFF; font-weight: bold; } QPushButton:hover { background: #0098FF; }");
    connect(searchBtn, &QPushButton::clicked, this, &KeywordSearchWidget::onSearch);

    auto* replaceBtn = new QPushButton(" 执行替换");
    replaceBtn->setAutoDefault(false);
    replaceBtn->setIcon(IconHelper::getIcon("edit", "#FFF", 16));
    replaceBtn->setStyleSheet("QPushButton { background: #D32F2F; border: none; border-radius: 4px; padding: 8px 20px; color: #FFF; font-weight: bold; } QPushButton:hover { background: #F44336; }");
    connect(replaceBtn, &QPushButton::clicked, this, &KeywordSearchWidget::onReplace);

    auto* undoBtn = new QPushButton(" 撤销替换");
    undoBtn->setAutoDefault(false);
    undoBtn->setIcon(IconHelper::getIcon("undo", "#EEE", 16));
    undoBtn->setStyleSheet("QPushButton { background: #3E3E42; border: none; border-radius: 4px; padding: 8px 20px; color: #EEE; } QPushButton:hover { background: #4E4E52; }");
    connect(undoBtn, &QPushButton::clicked, this, &KeywordSearchWidget::onUndo);

    auto* clearBtn = new QPushButton(" 清空日志");
    clearBtn->setAutoDefault(false);
    clearBtn->setIcon(IconHelper::getIcon("trash", "#EEE", 16));
    clearBtn->setStyleSheet("QPushButton { background: #3E3E42; border: none; border-radius: 4px; padding: 8px 20px; color: #EEE; } QPushButton:hover { background: #4E4E52; }");
    connect(clearBtn, &QPushButton::clicked, this, &KeywordSearchWidget::onClearLog);

    btnLayout->addWidget(searchBtn);
    btnLayout->addWidget(replaceBtn);
    btnLayout->addWidget(undoBtn);
    btnLayout->addWidget(clearBtn);
    btnLayout->addStretch();
    rightLayout->addLayout(btnLayout);

    // --- 日志展示区域 ---
    m_logDisplay = new QTextBrowser();
    m_logDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_logDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setUndoRedoEnabled(false);
    m_logDisplay->setOpenLinks(false);
    m_logDisplay->setOpenExternalLinks(false);
    m_logDisplay->setStyleSheet(
        "QTextBrowser { background: #1E1E1E; border: 1px solid #333; border-radius: 4px; color: #D4D4D4; font-family: 'Consolas', monospace; font-size: 12px; }"
    );
    connect(m_logDisplay, &QTextBrowser::anchorClicked, this, [](const QUrl& url) {
        if (url.scheme() == "file") {
            QString path = url.toLocalFile();
            QString nativePath = QDir::toNativeSeparators(path);
            QProcess::startDetached("explorer.exe", { "/select," + nativePath });
        }
    });
    rightLayout->addWidget(m_logDisplay, 1);

    // --- 状态栏 ---
    auto* statusLayout = new QVBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(4);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet("QProgressBar { background: #252526; border: none; } QProgressBar::chunk { background: #007ACC; }");
    m_progressBar->hide();
    
    m_statusLabel = new QLabel("就绪");
    m_statusLabel->setStyleSheet("color: #888; font-size: 11px;");
    
    statusLayout->addWidget(m_progressBar);
    statusLayout->addWidget(m_statusLabel);
    rightLayout->addLayout(statusLayout);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(1, 1);
}

void KeywordSearchWidget::onSidebarItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    m_pathEdit->setText(path);
}

void KeywordSearchWidget::showSidebarContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_sidebar->itemAt(pos);
    if (!item) return;
    
    m_sidebar->setCurrentItem(item);

    QMenu menu(this);
    menu.setWindowFlags(menu.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu.setAttribute(Qt::WA_TranslucentBackground);
    
    bool isPinned = item->data(Qt::UserRole + 1).toBool();
    QAction* pinAct = menu.addAction(IconHelper::getIcon("pin_vertical", isPinned ? "#007ACC" : "#AAA"), isPinned ? "取消置顶" : "置顶文件夹");
    QAction* removeAct = menu.addAction(IconHelper::getIcon("trash", "#E74C3C"), "取消收藏");
    
    QAction* selected = menu.exec(m_sidebar->mapToGlobal(pos));
    if (selected == pinAct) {
        bool newPinned = !isPinned;
        item->setData(Qt::UserRole + 1, newPinned);
        item->setIcon(IconHelper::getIcon("folder", newPinned ? "#007ACC" : "#F1C40F"));
        m_sidebar->sortItems(Qt::AscendingOrder);
        saveFavorites();
    } else if (selected == removeAct) {
        delete m_sidebar->takeItem(m_sidebar->row(item));
        saveFavorites();
    }
}

void KeywordSearchWidget::addFavorite(const QString& path, bool pinned) {
    // 检查是否已存在
    for (int i = 0; i < m_sidebar->count(); ++i) {
        if (m_sidebar->item(i)->data(Qt::UserRole).toString() == path) return;
    }

    QFileInfo fi(path);
    auto* item = new KeywordFavoriteItem(IconHelper::getIcon("folder", pinned ? "#007ACC" : "#F1C40F"), fi.fileName());
    item->setData(Qt::UserRole, path);
    item->setData(Qt::UserRole + 1, pinned);
    item->setToolTip(StringUtils::wrapToolTip(path));
    m_sidebar->addItem(item);
    m_sidebar->sortItems(Qt::AscendingOrder); 
    saveFavorites();
}

void KeywordSearchWidget::loadFavorites() {
    QSettings settings("SearchTool", "KeywordSearchFavorites");
    QVariant val = settings.value("list");
    if (val.typeId() == QMetaType::QStringList) {
        QStringList oldFavs = val.toStringList();
        for (const QString& path : oldFavs) {
            if (QDir(path).exists()) {
                addFavorite(path, false);
            }
        }
    } else {
        QVariantList favs = val.toList();
        for (const auto& v : std::as_const(favs)) {
            QVariantMap map = v.toMap();
            QString path = map["path"].toString();
            bool pinned = map["pinned"].toBool();
            if (QDir(path).exists()) {
                QFileInfo fi(path);
                auto* item = new KeywordFavoriteItem(IconHelper::getIcon("folder", pinned ? "#007ACC" : "#F1C40F"), fi.fileName());
                item->setData(Qt::UserRole, path);
                item->setData(Qt::UserRole + 1, pinned);
                item->setToolTip(StringUtils::wrapToolTip(path));
                m_sidebar->addItem(item);
            }
        }
    }
    m_sidebar->sortItems(Qt::AscendingOrder);
}

void KeywordSearchWidget::saveFavorites() {
    QVariantList favs;
    for (int i = 0; i < m_sidebar->count(); ++i) {
        QVariantMap map;
        map["path"] = m_sidebar->item(i)->data(Qt::UserRole).toString();
        map["pinned"] = m_sidebar->item(i)->data(Qt::UserRole + 1).toBool();
        favs << map;
    }
    QSettings settings("SearchTool", "KeywordSearchFavorites");
    settings.setValue("list", favs);
}

void KeywordSearchWidget::onBrowseFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, "选择搜索目录");
    if (!folder.isEmpty()) {
        m_pathEdit->setText(folder);
    }
}

bool KeywordSearchWidget::isTextFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray chunk = file.read(1024);
    file.close();

    if (chunk.isEmpty()) return true;
    if (chunk.contains('\0')) return false;

    return true;
}

void KeywordSearchWidget::log(const QString& msg, const QString& type) {
    QString color = "#D4D4D4";
    if (type == "success") color = "#6A9955";
    else if (type == "error") color = "#F44747";
    else if (type == "header") color = "#007ACC";
    else if (type == "file") color = "#E1523D";

    QString html = QString("<span style='color:%1;'>%2</span>").arg(color, msg.toHtmlEscaped());
    // 如果是文件，添加自定义属性以便识别
    if (type == "file") {
        html = QString("<a href=\"%1\" style=\"color:%2; text-decoration: underline;\">📄 文件: %3</a>")
                .arg(QUrl::fromLocalFile(msg).toString(), color, msg.toHtmlEscaped());
    }

    m_logDisplay->append(html);
}

void KeywordSearchWidget::onSearch() {
    QString rootDir = m_pathEdit->text().trimmed();
    QString keyword = m_searchEdit->text().trimmed();
    QString replaceText = m_replaceEdit->text().trimmed();
    if (rootDir.isEmpty() || keyword.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color: #e74c3c;'>✖ 目录和查找内容不能为空!</b>"), this);
        return;
    }

    // 保存历史记录
    addHistoryEntry(Path, rootDir);
    addHistoryEntry(Keyword, keyword);
    if (!replaceText.isEmpty()) {
        addHistoryEntry(Replace, replaceText);
    }

    m_logDisplay->clear();
    m_progressBar->show();
    m_progressBar->setRange(0, 0);
    m_statusLabel->setText("正在搜索...");

    QString filter = m_filterEdit->text();
    bool caseSensitive = m_caseCheck->isChecked();

    (void)QtConcurrent::run([this, rootDir, keyword, filter, caseSensitive]() {
        int foundFiles = 0;
        int scannedFiles = 0;

        QStringList filters;
        if (!filter.isEmpty()) {
            filters = filter.split(QRegularExpression("[,\\s;]+"), Qt::SkipEmptyParts);
        }

        QDirIterator it(rootDir, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            
            // 过滤目录
            bool skip = false;
            for (const QString& ignore : m_ignoreDirs) {
                if (filePath.contains("/" + ignore + "/") || filePath.contains("\\" + ignore + "\\")) {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            // 过滤文件名
            if (!filters.isEmpty()) {
                bool matchFilter = false;
                QString fileName = QFileInfo(filePath).fileName();
                for (const QString& f : filters) {
                    QRegularExpression re(QRegularExpression::wildcardToRegularExpression(f));
                    if (re.match(fileName).hasMatch()) {
                        matchFilter = true;
                        break;
                    }
                }
                if (!matchFilter) continue;
            }

            if (!isTextFile(filePath)) continue;

            scannedFiles++;
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                file.close();

                Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                if (content.contains(keyword, cs)) {
                    foundFiles++;
                    int count = content.count(keyword, cs);
                    QMetaObject::invokeMethod(this, [this, filePath, count]() {
                        log(filePath, "file");
                        log(QString("   匹配次数: %1\n").arg(count));
                    });
                }
            }
        }

        QMetaObject::invokeMethod(this, [this, scannedFiles, foundFiles, keyword, caseSensitive]() {
            log(QString("\n搜索完成! 扫描 %1 个文件，找到 %2 个匹配\n").arg(scannedFiles).arg(foundFiles), "success");
            m_statusLabel->setText(QString("完成: 找到 %1 个文件").arg(foundFiles));
            m_progressBar->hide();
            highlightResult(keyword);
        });
    });
}

void KeywordSearchWidget::highlightResult(const QString& keyword) {
    if (keyword.isEmpty()) return;
}

void KeywordSearchWidget::onReplace() {
    QString rootDir = m_pathEdit->text().trimmed();
    QString keyword = m_searchEdit->text().trimmed();
    QString replaceText = m_replaceEdit->text().trimmed();
    if (rootDir.isEmpty() || keyword.isEmpty()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color: #e74c3c;'>✖ 目录和查找内容不能为空!</b>"), this);
        return;
    }

    // 保存历史记录
    addHistoryEntry(Path, rootDir);
    addHistoryEntry(Keyword, keyword);
    if (!replaceText.isEmpty()) {
        addHistoryEntry(Replace, replaceText);
    }

    // 遵从非阻塞规范，直接执行替换（已有备份机制）
    QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color: #007acc;'>ℹ 正在开始批量替换...</b>"), this);

    m_progressBar->show();
    m_progressBar->setRange(0, 0);
    m_statusLabel->setText("正在替换...");

    QString filter = m_filterEdit->text();
    bool caseSensitive = m_caseCheck->isChecked();

    (void)QtConcurrent::run([this, rootDir, keyword, replaceText, filter, caseSensitive]() {
        int modifiedFiles = 0;
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString backupDirName = "_backup_" + timestamp;
        QDir root(rootDir);
        root.mkdir(backupDirName);
        m_lastBackupPath = root.absoluteFilePath(backupDirName);

        QStringList filters;
        if (!filter.isEmpty()) {
            filters = filter.split(QRegularExpression("[,\\s;]+"), Qt::SkipEmptyParts);
        }

        QDirIterator it(rootDir, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            if (filePath.contains(backupDirName)) continue;

            // 过滤目录和文件名（逻辑同搜索）
            bool skip = false;
            for (const QString& ignore : m_ignoreDirs) {
                if (filePath.contains("/" + ignore + "/") || filePath.contains("\\" + ignore + "\\")) {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            if (!filters.isEmpty()) {
                bool matchFilter = false;
                QString fileName = QFileInfo(filePath).fileName();
                for (const QString& f : filters) {
                    QRegularExpression re(QRegularExpression::wildcardToRegularExpression(f));
                    if (re.match(fileName).hasMatch()) {
                        matchFilter = true;
                        break;
                    }
                }
                if (!matchFilter) continue;
            }

            if (!isTextFile(filePath)) continue;

            QFile file(filePath);
            if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                
                Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                if (content.contains(keyword, cs)) {
                    // 备份
                    QString fileName = QFileInfo(filePath).fileName();
                    QFile::copy(filePath, m_lastBackupPath + "/" + fileName + ".bak");

                    // 替换
                    QString newContent;
                    if (caseSensitive) {
                        newContent = content.replace(keyword, replaceText);
                    } else {
                        newContent = content.replace(QRegularExpression(QRegularExpression::escape(keyword), QRegularExpression::CaseInsensitiveOption), replaceText);
                    }

                    file.resize(0);
                    in << newContent;
                    modifiedFiles++;
                    QMetaObject::invokeMethod(this, [this, fileName]() {
                        log("已修改: " + fileName, "success");
                    });
                }
                file.close();
            }
        }

        QMetaObject::invokeMethod(this, [this, modifiedFiles]() {
            log(QString("\n替换完成! 修改了 %1 个文件").arg(modifiedFiles), "success");
            m_statusLabel->setText(QString("完成: 修改了 %1 个文件").arg(modifiedFiles));
            m_progressBar->hide();
            QToolTip::showText(QCursor::pos(), 
                StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>✔ 已修改 %1 个文件 (备份于 %2)</b>")
                .arg(modifiedFiles).arg(QFileInfo(m_lastBackupPath).fileName())), this);
        });
    });
}

void KeywordSearchWidget::onUndo() {
    if (m_lastBackupPath.isEmpty() || !QDir(m_lastBackupPath).exists()) {
        QToolTip::showText(QCursor::pos(), StringUtils::wrapToolTip("<b style='color: #e74c3c;'>✖ 未找到有效的备份目录！</b>"), this);
        return;
    }

    int restored = 0;
    QDir backupDir(m_lastBackupPath);
    QStringList baks = backupDir.entryList({"*.bak"});
    
    QString rootDir = m_pathEdit->text();

    for (const QString& bak : baks) {
        QString origName = bak.left(bak.length() - 4);
        
        // 在根目录下寻找原始文件（简化策略：找同名文件）
        QDirIterator it(rootDir, {origName}, QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext()) {
            QString targetPath = it.next();
            if (QFile::remove(targetPath)) {
                if (QFile::copy(backupDir.absoluteFilePath(bak), targetPath)) {
                    restored++;
                }
            }
        }
    }

    log(QString("↶ 撤销完成，已恢复 %1 个文件\n").arg(restored), "success");
    QToolTip::showText(QCursor::pos(), 
        StringUtils::wrapToolTip(QString("<b style='color: #2ecc71;'>✔ 已恢复 %1 个文件</b>").arg(restored)), this);
}

void KeywordSearchWidget::onClearLog() {
    m_logDisplay->clear();
    m_statusLabel->setText("就绪");
}

void KeywordSearchWidget::onResultDoubleClicked(const QModelIndex& index) {
}

void KeywordSearchWidget::onSwapSearchReplace() {
    QString searchTxt = m_searchEdit->text();
    QString replaceTxt = m_replaceEdit->text();
    m_searchEdit->setText(replaceTxt);
    m_replaceEdit->setText(searchTxt);
}

void KeywordSearchWidget::addHistoryEntry(HistoryType type, const QString& text) {
    if (text.isEmpty()) return;
    QString key = "keywordList";
    if (type == Path) key = "pathList";
    else if (type == Replace) key = "replaceList";

    QSettings settings("SearchTool", "KeywordSearchHistory");
    QStringList history = settings.value(key).toStringList();
    history.removeAll(text);
    history.prepend(text);
    while (history.size() > 10) history.removeLast();
    settings.setValue(key, history);
}

void KeywordSearchWidget::onShowHistory() {
    auto* edit = qobject_cast<ClickableLineEdit*>(sender());
    if (!edit) return;

    KeywordSearchHistoryPopup::Type type = KeywordSearchHistoryPopup::Keyword;
    if (edit == m_pathEdit) type = KeywordSearchHistoryPopup::Path;
    else if (edit == m_replaceEdit) type = KeywordSearchHistoryPopup::Replace;
    
    auto* popup = new KeywordSearchHistoryPopup(this, edit, type);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->showAnimated();
}

// ----------------------------------------------------------------------------
// KeywordSearchWindow 实现
// ----------------------------------------------------------------------------
KeywordSearchWindow::KeywordSearchWindow(QWidget* parent) : FramelessDialog("搜索关键字", parent) {
    resize(1000, 700);
    m_searchWidget = new KeywordSearchWidget(m_contentArea);
    auto* layout = new QVBoxLayout(m_contentArea);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_searchWidget);
}

KeywordSearchWindow::~KeywordSearchWindow() {
}

void KeywordSearchWindow::hideEvent(QHideEvent* event) {
    FramelessDialog::hideEvent(event);
}

#include "KeywordSearchWindow.moc"
```

## 文件: `src/windows/KeywordSearchWindow.h`

```cpp
#ifndef KEYWORDSEARCHWINDOW_H
#define KEYWORDSEARCHWINDOW_H

#include "FramelessDialog.h"
#include "../widgets/ClickableLineEdit.h"
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTextBrowser>
#include <QProgressBar>
#include <QLabel>
#include <QListWidget>
#include <QSplitter>

class KeywordSidebarListWidget;

/**
 * @brief 关键字搜索核心组件
 */
class KeywordSearchWidget : public QWidget {
    Q_OBJECT
public:
    explicit KeywordSearchWidget(QWidget* parent = nullptr);
    ~KeywordSearchWidget();

private slots:
    void onBrowseFolder();
    void onSidebarItemClicked(QListWidgetItem* item);
    void showSidebarContextMenu(const QPoint& pos);
    void addFavorite(const QString& path, bool pinned = false);
    void onSearch();
    void onReplace();
    void onUndo();
    void onClearLog();
    void onResultDoubleClicked(const QModelIndex& index);
    void onShowHistory();
    void onSwapSearchReplace();

private:
    void initUI();
    void setupStyles();
    void loadFavorites();
    void saveFavorites();
    
    // 历史记录管理
    enum HistoryType { Path, Keyword, Replace };
    void addHistoryEntry(HistoryType type, const QString& text);
    bool isTextFile(const QString& filePath);
    void log(const QString& msg, const QString& type = "info");
    void highlightResult(const QString& keyword);

    QListWidget* m_sidebar;
    ClickableLineEdit* m_pathEdit;
    QLineEdit* m_filterEdit;
    ClickableLineEdit* m_searchEdit;
    ClickableLineEdit* m_replaceEdit;
    QCheckBox* m_caseCheck;
    QTextBrowser* m_logDisplay;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;

    QString m_lastBackupPath;
    QStringList m_ignoreDirs;
};

/**
 * @brief 关键字搜索窗口：封装了 KeywordSearchWidget
 */
class KeywordSearchWindow : public FramelessDialog {
    Q_OBJECT
public:
    explicit KeywordSearchWindow(QWidget* parent = nullptr);
    ~KeywordSearchWindow();

protected:
    void hideEvent(QHideEvent* event) override;

private:
    KeywordSearchWidget* m_searchWidget;
};

#endif // KEYWORDSEARCHWINDOW_H
```

## 文件: `src/main.cpp`

```cpp
#include <QApplication>
#include <QFile>
#include <QIcon>
#include "windows/SearchAppWindow.h"
#include "windows/SystemTray.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("SearchTool");
    a.setOrganizationName("SearchToolDev");
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
```

## 文件: `src/windows/SearchAppWindow.cpp`

```cpp
#include "SearchAppWindow.h"
#include "FileSearchWindow.h"
#include "KeywordSearchWindow.h"
#include "../utils/IconHelper.h"
#include <QVBoxLayout>

SearchAppWindow::SearchAppWindow(QWidget* parent) 
    : FramelessDialog("搜索主程序", parent)
{
    setObjectName("SearchTool_SearchAppWindow");
    resize(1100, 750);
    setupStyles();
    initUI();
}

SearchAppWindow::~SearchAppWindow() {
}

void SearchAppWindow::setupStyles() {
    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #333;
            background: #1e1e1e;
            top: -1px;
            border-radius: 4px;
        }
        QTabBar::tab {
            background: #2D2D30;
            color: #AAA;
            padding: 10px 20px;
            border: 1px solid #333;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            margin-right: 2px;
        }
        QTabBar::tab:hover {
            background: #3E3E42;
            color: #EEE;
        }
        QTabBar::tab:selected {
            background: #1e1e1e;
            color: #007ACC;
            border-bottom: 1px solid #1e1e1e;
            font-weight: bold;
        }
    )");
}

void SearchAppWindow::initUI() {
    auto* layout = new QVBoxLayout(m_contentArea);
    layout->setContentsMargins(10, 5, 10, 10);
    layout->addWidget(m_tabWidget);

    m_fileSearchWidget = new FileSearchWidget();
    m_keywordSearchWidget = new KeywordSearchWidget();

    m_tabWidget->addTab(m_fileSearchWidget, IconHelper::getIcon("folder", "#AAA"), "文件查找");
    m_tabWidget->addTab(m_keywordSearchWidget, IconHelper::getIcon("find_keyword", "#AAA"), "关键字查找");
}

void SearchAppWindow::resizeEvent(QResizeEvent* event) {
    FramelessDialog::resizeEvent(event);
}
```

## 文件: `src/windows/SearchAppWindow.h`

```cpp
#ifndef SEARCHAPPWINDOW_H
#define SEARCHAPPWINDOW_H

#include "FramelessDialog.h"
#include <QTabWidget>

class FileSearchWidget;
class KeywordSearchWidget;

class SearchAppWindow : public FramelessDialog {
    Q_OBJECT
public:
    explicit SearchAppWindow(QWidget* parent = nullptr);
    ~SearchAppWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void initUI();
    void setupStyles();

    QTabWidget* m_tabWidget;
    FileSearchWidget* m_fileSearchWidget;
    KeywordSearchWidget* m_keywordSearchWidget;
};

#endif // SEARCHAPPWINDOW_H
```

## 文件: `src/utils/StringUtils.h`

```cpp
#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>
#include <QTextDocument>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>
#include <QSettings>
#include <QVariantList>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class StringUtils {
public:
    /**
     * @brief 智能语言拆分：中文作为标题，非中文作为内容
     */
    static void smartSplitLanguage(const QString& text, QString& title, QString& content) {
        QString trimmedText = text.trimmed();
        if (trimmedText.isEmpty()) {
            title = "新笔记";
            content = "";
            return;
        }

        static QRegularExpression chineseRegex("[\\x{4e00}-\\x{9fa5}]+");
        static QRegularExpression otherRegex("[^\\x{4e00}-\\x{9fa5}\\s\\p{P}]+");

        bool hasChinese = trimmedText.contains(chineseRegex);
        bool hasOther = trimmedText.contains(otherRegex);

        if (hasChinese && hasOther) {
            QStringList chineseBlocks;
            QRegularExpressionMatchIterator i = chineseRegex.globalMatch(trimmedText);
            while (i.hasNext()) {
                chineseBlocks << i.next().captured();
            }
            title = chineseBlocks.join(" ").simplified();
            if (title.isEmpty()) title = "未命名";

            QString remaining = trimmedText;
            remaining.replace(chineseRegex, " ");
            content = remaining.simplified();
            
            if (content.isEmpty()) content = trimmedText;
        } else {
            QStringList lines = trimmedText.split('\n', Qt::SkipEmptyParts);
            if (!lines.isEmpty()) {
                title = lines[0].trimmed();
                if (title.length() > 60) title = title.left(57) + "...";
                content = trimmedText;
            } else {
                title = "新笔记";
                content = trimmedText;
            }
        }
    }

    static bool containsChinese(const QString& text) {
        static QRegularExpression chineseRegex("[\\x{4e00}-\\x{9fa5}]+");
        return text.contains(chineseRegex);
    }

    static QList<QPair<QString, QString>> smartSplitPairs(const QString& text) {
        QList<QPair<QString, QString>> results;
        QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        
        if (lines.isEmpty()) return results;

        if (lines.size() > 0 && lines.size() % 2 == 0) {
            for (int i = 0; i < lines.size(); i += 2) {
                QString line1 = lines[i].trimmed();
                QString line2 = lines[i+1].trimmed();
                
                bool c1 = containsChinese(line1);
                bool c2 = containsChinese(line2);
                
                if (c1 && !c2) {
                    results.append({line1, line2});
                } else if (!c1 && c2) {
                    results.append({line2, line1});
                } else {
                    results.append({line1, line2});
                }
            }
        } else {
            QString title, content;
            smartSplitLanguage(text, title, content);
            results.append({title, content});
        }
        
        return results;
    }

public:
    static bool isHtml(const QString& text) {
        return text.contains("<!DOCTYPE HTML") || text.contains("<html>") || text.contains("<style");
    }

    static QString htmlToPlainText(const QString& html) {
        if (!isHtml(html)) return html;
        QTextDocument doc;
        doc.setHtml(html);
        return doc.toPlainText();
    }

    static QString convertChineseVariant(const QString& text, bool toSimplified) {
#ifdef Q_OS_WIN
        if (text.isEmpty()) return text;
        
        std::wstring wstr = text.toStdWString();
        DWORD flags = toSimplified ? LCMAP_SIMPLIFIED_CHINESE : LCMAP_TRADITIONAL_CHINESE;
        
        int size = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, flags, wstr.c_str(), -1, NULL, 0, NULL, NULL, 0);
        if (size > 0) {
            std::vector<wchar_t> buffer(size);
            LCMapStringEx(LOCALE_NAME_USER_DEFAULT, flags, wstr.c_str(), -1, buffer.data(), size, NULL, NULL, 0);
            return QString::fromWCharArray(buffer.data());
        }
#endif
        return text;
    }

    static QString getToolTipStyle() {
        return "QToolTip { color: #ffffff; background-color: #2D2D2D; border: 1px solid #555555; border-radius: 6px; padding: 5px 10px; }";
    }

    static QString wrapToolTip(const QString& text) {
        if (text.isEmpty()) return text;
        if (text.startsWith("<html>")) return text;
        return QString("<html><span style='white-space:nowrap;'>%1</span></html>").arg(text);
    }
};

#endif // STRINGUTILS_H
```

## 文件: `src/ui/StringUtils.h`

```cpp
#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>
#include <QTextDocument>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>
#include <QSettings>
#include <QVariantList>
#include <vector>
#include "../core/ClipboardMonitor.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class StringUtils {
public:
    /**
     * @brief 智能语言拆分：中文作为标题，非中文作为内容
     */
    static void smartSplitLanguage(const QString& text, QString& title, QString& content) {
        QString trimmedText = text.trimmed();
        if (trimmedText.isEmpty()) {
            title = "新笔记";
            content = "";
            return;
        }

        // 匹配中文字符范围
        static QRegularExpression chineseRegex("[\\x{4e00}-\\x{9fa5}]+");
        // 匹配非中文且非空白非标点的字符（识别泰文、英文等）
        static QRegularExpression otherRegex("[^\\x{4e00}-\\x{9fa5}\\s\\p{P}]+");

        bool hasChinese = trimmedText.contains(chineseRegex);
        bool hasOther = trimmedText.contains(otherRegex);

        if (hasChinese && hasOther) {
            // 提取所有中文块作为标题
            QStringList chineseBlocks;
            QRegularExpressionMatchIterator i = chineseRegex.globalMatch(trimmedText);
            while (i.hasNext()) {
                chineseBlocks << i.next().captured();
            }
            title = chineseBlocks.join(" ").simplified();
            if (title.isEmpty()) title = "未命名";

            // 移除中文块后的剩余部分作为内容
            QString remaining = trimmedText;
            remaining.replace(chineseRegex, " ");
            content = remaining.simplified();
            
            // 如果拆分后内容为空（例如全是标点），则保留全文
            if (content.isEmpty()) content = trimmedText;
        } else {
            // 单一语种或无法识别：首行作为标题，全文作为内容
            QStringList lines = trimmedText.split('\n', Qt::SkipEmptyParts);
            if (!lines.isEmpty()) {
                title = lines[0].trimmed();
                if (title.length() > 60) title = title.left(57) + "...";
                content = trimmedText;
            } else {
                title = "新笔记";
                content = trimmedText;
            }
        }
    }

    /**
     * @brief 智能识别语言：判断文本是否包含中文
     */
    static bool containsChinese(const QString& text) {
        static QRegularExpression chineseRegex("[\\x{4e00}-\\x{9fa5}]+");
        return text.contains(chineseRegex);
    }

    /**
     * @brief 偶数行配对拆分：每两行为一组
     * 规则：含中文的行为标题，若同语种则第一行为标题。
     */
    static QList<QPair<QString, QString>> smartSplitPairs(const QString& text) {
        QList<QPair<QString, QString>> results;
        QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        
        if (lines.isEmpty()) return results;

        // 如果是偶数行，执行配对逻辑
        if (lines.size() > 0 && lines.size() % 2 == 0) {
            for (int i = 0; i < lines.size(); i += 2) {
                QString line1 = lines[i].trimmed();
                QString line2 = lines[i+1].trimmed();
                
                bool c1 = containsChinese(line1);
                bool c2 = containsChinese(line2);
                
                if (c1 && !c2) {
                    results.append({line1, line2});
                } else if (!c1 && c2) {
                    results.append({line2, line1});
                } else {
                    // 同语种，第一行为标题
                    results.append({line1, line2});
                }
            }
        } else {
            // 奇数行或单行，沿用之前的单条逻辑
            QString title, content;
            smartSplitLanguage(text, title, content);
            results.append({title, content});
        }
        
        return results;
    }

public:
    static bool isHtml(const QString& text) {
        return text.contains("<!DOCTYPE HTML") || text.contains("<html>") || text.contains("<style");
    }

    static QString htmlToPlainText(const QString& html) {
        if (!isHtml(html)) return html;
        QTextDocument doc;
        doc.setHtml(html);
        return doc.toPlainText();
    }

    static void copyNoteToClipboard(const QString& content) {
        ClipboardMonitor::instance().skipNext();
        QMimeData* mimeData = new QMimeData();
        if (isHtml(content)) {
            mimeData->setHtml(content);
            mimeData->setText(htmlToPlainText(content));
        } else {
            mimeData->setText(content);
        }
        QApplication::clipboard()->setMimeData(mimeData);
    }

    /**
     * @brief 简繁转换 (利用 Windows 原生 API)
     * @param toSimplified true 为转简体，false 为转繁体
     */
    static QString convertChineseVariant(const QString& text, bool toSimplified) {
#ifdef Q_OS_WIN
        if (text.isEmpty()) return text;
        
        // 转换为宽字符
        std::wstring wstr = text.toStdWString();
        DWORD flags = toSimplified ? LCMAP_SIMPLIFIED_CHINESE : LCMAP_TRADITIONAL_CHINESE;
        
        // 第一次调用获取长度
        int size = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, flags, wstr.c_str(), -1, NULL, 0, NULL, NULL, 0);
        if (size > 0) {
            std::vector<wchar_t> buffer(size);
            // 第二次调用执行转换
            LCMapStringEx(LOCALE_NAME_USER_DEFAULT, flags, wstr.c_str(), -1, buffer.data(), size, NULL, NULL, 0);
            return QString::fromWCharArray(buffer.data());
        }
#endif
        return text;
    }

    /**
     * @brief 获取全局统一的 ToolTip QSS 样式字符串
     */
    static QString getToolTipStyle() {
        // [CRITICAL] 核心修复：利用 margin 使背景收缩。
        // 通过设置 QToolTip 本身背景为透明，并为内容区域设置黑色背景和圆角，
        // margin: 2px 确保了黑色背景比透明窗口小一圈，从而彻底消除圆角边缘的直角溢出。
        return "QToolTip { background-color: #2D2D2D; color: #ffffff; border: 1px solid #555555; border-radius: 6px; padding: 5px; margin: 2px; }";
    }

    /**
     * @brief 包装 ToolTip 为富文本格式，强制触发 QSS 样式渲染
     */
    static QString wrapToolTip(const QString& text) {
        if (text.isEmpty()) return text;
        // [CRITICAL] 使用 <span> 包装并检查 ID 防止重复。禁止在 HTML 中使用 div 容器定义圆角，
        // 因为 Qt 富文本引擎不支持 border-radius。圆角必须由 QSS 在组件级别实现。
        if (text.contains("id='qtooltip_inner'")) return text;

        QString content = text;
        if (content.startsWith("<html>")) {
            content.remove("<html>");
            content.remove("</html>");
        }

        return QString("<html><span id='qtooltip_inner'>%1</span></html>").arg(content);
    }

    /**
     * @brief 记录最近访问或使用的分类
     */
    static void recordRecentCategory(int catId) {
        if (catId <= 0) return;
        QSettings settings("SearchTool", "QuickWindow");
        QVariantList recentCats = settings.value("recentCategories").toList();
        
        // 转换为 int 列表方便操作
        QList<int> ids;
        for(const auto& v : recentCats) ids << v.toInt();
        
        ids.removeAll(catId);
        ids.prepend(catId);
        
        // 限制为最近 10 个
        while (ids.size() > 10) ids.removeLast();
        
        QVariantList result;
        for(int id : ids) result << id;
        settings.setValue("recentCategories", result);
        settings.sync();
    }

    /**
     * @brief 获取最近访问或使用的分类 ID 列表
     */
    static QVariantList getRecentCategories() {
        QSettings settings("SearchTool", "QuickWindow");
        return settings.value("recentCategories").toList();
    }
};

#endif // STRINGUTILS_H
```

## 文件: `src/utils/SvgIcons.h`

```cpp
#ifndef SVGICONS_H
#define SVGICONS_H

#include <QString>
#include <QMap>

namespace SvgIcons {
    inline const QMap<QString, QString> icons = {
        {"text", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" y1="6" x2="20" y2="6"></line><line x1="4" y1="11" x2="14" y2="11"></line><line x1="4" y1="16" x2="20" y2="16"></line><line x1="4" y1="21" x2="14" y2="21"></line></svg>)svg"},
        {"untagged", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z"></path><line x1="7" y1="7" x2="7.01" y2="7"></line><path d="M11 11l4 4m0-4l-4 4" /></svg>)svg"},
        {"tag", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z"></path><line x1="7" y1="7" x2="7.01" y2="7"></line></svg>)svg"},
        {"file", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line><line x1="10" y1="9" x2="8" y2="9"></line></svg>)svg"},
        {"code", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="16 18 22 12 16 6"></polyline><polyline points="8 6 2 12 8 18"></polyline></svg>)svg"},
        {"link", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"></path><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"></path></svg>)svg"},
        {"image", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><circle cx="8.5" cy="8.5" r="1.5"></circle><polyline points="21 15 16 10 5 21"></polyline></svg>)svg"},
        {"branch", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="5" r="3"></circle><path d="M12 8v5"></path><path d="M12 13l-5 4"></path><path d="M12 13l5 4"></path><circle cx="7" cy="19" r="3"></circle><circle cx="17" cy="19" r="3"></circle></svg>)svg"},
        {"category", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="8" y="2" width="8" height="6" rx="1"></rect><path d="M12 8 v3"></path><path d="M12 11 h-6"></path><path d="M12 11 h6"></path><rect x="2" y="13" width="8" height="5" rx="1"></rect><rect x="14" y="13" width="8" height="5" rx="1"></rect><circle cx="12" cy="5" r="1" fill="currentColor"></circle><circle cx="6" cy="15.5" r="1" fill="currentColor"></circle><circle cx="18" cy="15.5" r="1" fill="currentColor"></circle></svg>)svg"},
        {"uncategorized", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M5 8 C5 4 10 4 10 8 C10 11 7 12 7 14" /><circle cx="7" cy="19" r="1" fill="currentColor" stroke="none"/><path d="M14 5 v14" /><path d="M14 6 h3" /> <circle cx="20" cy="6" r="2" /><path d="M14 12 h3" /> <circle cx="20" cy="12" r="2" /><path d="M14 18 h3" /> <circle cx="20" cy="18" r="2" /></svg>)svg"},
        {"trash", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" /><line x1="10" y1="11" x2="10" y2="17" /><line x1="14" y1="11" x2="14" y2="17" /></svg>)svg"},
        {"refresh", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21.5 2v6h-6"></path><path d="M2.5 22v-6h6"></path><path d="M21.5 8A10 10 0 0 0 6 3.5l-3.5 4"></path><path d="M2.5 16A10 10 0 0 0 18 20.5l3.5-4"></path><circle cx="12" cy="12" r="1.5" fill="currentColor" opacity="0.3"></circle></svg>)svg"},
        {"search", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>)svg"},
        {"add", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="12" y1="5" x2="12" y2="19"></line><line x1="5" y1="12" x2="19" y2="12"></line></svg>)svg"},
        {"edit", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"></path><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"></path></svg>)svg"},
        {"bookmark", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"></path></svg>)svg"},
        {"star", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"></polygon></svg>)svg"},
        {"location", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"></path><circle cx="12" cy="10" r="3"></circle></svg>)svg"},
        {"pin", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 12V6H8v6l-2 2v2h5v8l1 1 1-1v-8h5v-2l-2-2z"></path></svg>)svg"},
        {"lock", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>)svg"},
        {"lock_secure", R"svg(<svg viewBox="0 0 24 24" fill="currentColor"><path fill-rule="evenodd" d="M12 2a5 5 0 0 0-5 5v3H6a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8a2 2 0 0 0-2-2h-1V7a5 5 0 0 0-5-5zM9 10V7a3 3 0 0 1 6 0v3H9zm3 4a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3zm-0.75 3h1.5v3h-1.5v-3z" clip-rule="evenodd"/></svg>)svg"},
        // 专门用于"密码生成器"的图标：锁+密码位样式
        {"password_generator", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M16 11V7a4 4 0 0 0-8 0v4" />
            <rect x="3" y="11" width="13" height="10" rx="2" />
            <rect x="11" y="14" width="11" height="7" rx="3.5" />
            <rect x="13.5" y="16.5" width="1.5" height="1.5" fill="currentColor" stroke="none" />
            <rect x="16.25" y="16.5" width="1.5" height="1.5" fill="currentColor" stroke="none" />
            <rect x="19" y="16.5" width="1.5" height="1.5" fill="currentColor" stroke="none" />
        </svg>)svg"},
        {"eye", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>)svg"},
        {"toolbox", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="7" width="20" height="14" rx="2" ry="2"></rect><path d="M6 7V5a2 2 0 0 1 2-2h8a2 2 0 0 1 2 2v2"></path><line x1="12" y1="12" x2="12" y2="16"></line><line x1="8" y1="12" x2="8" y2="16"></line><line x1="16" y1="12" x2="16" y2="16"></line></svg>)svg"},
        {"today", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="4" width="18" height="18" rx="2" ry="2"></rect><line x1="16" y1="2" x2="16" y2="6"></line><line x1="8" y1="2" x2="8" y2="6"></line><line x1="3" y1="10" x2="21" y2="10"></line></svg>)svg"},
        {"all_data", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><ellipse cx="12" cy="5" rx="9" ry="3"></ellipse><path d="M21 12c0 1.66-4 3-9 3s-9-1.34-9-3"></path><path d="M3 5v14c0 1.66 4 3 9 3s9-1.34 9-3V5"></path></svg>)svg"},
        {"sidebar", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><line x1="9" y1="3" x2="9" y2="21"></line></svg>)svg"},
        {"sidebar_right", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><line x1="15" y1="3" x2="15" y2="21"></line></svg>)svg"},
        {"nav_first", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="11 17 6 12 11 7"></polyline><polyline points="18 17 13 12 18 7"></polyline></svg>)svg"},
        {"nav_prev", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 18 9 12 15 6"></polyline></svg>)svg"},
        {"nav_next", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"></polyline></svg>)svg"},
        {"nav_last", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="13 17 18 12 13 7"></polyline><polyline points="6 17 11 12 6 7"></polyline></svg>)svg"},
        {"undo", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 7v6h6"></path><path d="M21 17a9 9 0 0 0-9-9 9 9 0 0 0-6 2.3L3 13"></path></svg>)svg"},
        {"coffee", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 8h1a4 4 0 0 1 0 8h-1"/><path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/><line x1="6" y1="1" x2="6" y2="4"/><line x1="10" y1="1" x2="10" y2="4"/><line x1="14" y1="1" x2="14" y2="4"/></svg>)svg"},
        {"grid", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/><line x1="3" y1="9" x2="21" y2="9"/><line x1="3" y1="15" x2="21" y2="15"/><line x1="9" y1="3" x2="9" y2="21"/><line x1="15" y1="3" x2="15" y2="21"/></svg>)svg"},
        {"book", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20v2H6.5A2.5 2.5 0 0 1 4 19.5z"/><path d="M4 5.5A2.5 2.5 0 0 1 6.5 3H20v2H6.5A2.5 2.5 0 0 1 4 5.5z"/></svg>)svg"},
        {"leaf", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 12c0-4.42-3.58-8-8-8S4 7.58 4 12s3.58 8 8 8 8-3.58 8-8z"/><path d="M12 2a10 10 0 0 0-10 10h20a10 10 0 0 0-10-10z"/></svg>)svg"},
        {"book_open", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>)svg"},
        {"redo", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 7v6h-6"></path><path d="M3 17a9 9 0 0 1 9-9 9 9 0 0 1 6 2.3l3 2.7"></path></svg>)svg"},
        {"list_ul", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="8" y1="6" x2="21" y2="6"></line><line x1="8" y1="12" x2="21" y2="12"></line><line x1="8" y1="18" x2="21" y2="18"></line><line x1="3" y1="6" x2="3.01" y2="6"></line><line x1="3" y1="12" x2="3.01" y2="12"></line><line x1="3" y1="18" x2="3.01" y2="18"></line></svg>)svg"},
        {"list_ol", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="10" y1="6" x2="21" y2="6"></line><line x1="10" y1="12" x2="21" y2="12"></line><line x1="10" y1="18" x2="21" y2="18"></line><path d="M4 6h1v4"></path><path d="M4 10h2"></path><path d="M6 18H4c0-1 2-2 2-3s-1-1.5-2-1"></path></svg>)svg"},
        {"todo", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><path d="M9 12l2 2 4-4"></path></svg>)svg"},
        {"close", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>)svg"},
        {"save", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>)svg"},
        {"filter", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 11l3 3L22 4"/><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"/></svg>)svg"},
        {"select", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 11 12 14 22 4"></polyline><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"></path></svg>)svg"},
        {"grip_diagonal", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="19" cy="19" r="1"></circle><circle cx="19" cy="14" r="1"></circle><circle cx="14" cy="19" r="1"></circle><circle cx="19" cy="9" r="1"></circle><circle cx="14" cy="14" r="1"></circle><circle cx="9" cy="19" r="1"></circle></svg>)svg"},
        {"folder", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path></svg>)svg"},
        {"file_managed", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><path d="M12 18h6v3h-6z" fill="currentColor" stroke="none" /><path d="M12 15h6v1h-6z" fill="currentColor" stroke="none" /></svg>)svg"},
        {"folder_managed", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path><path d="M12 18h8v3h-8z" fill="currentColor" stroke="none" /><path d="M12 15h8v1h-8z" fill="currentColor" stroke="none" /></svg>)svg"},
        {"settings", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>)svg"},
        {"calendar", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="4" width="18" height="18" rx="2" ry="2"></rect><line x1="16" y1="2" x2="16" y2="6"></line><line x1="8" y1="2" x2="8" y2="6"></line><line x1="3" y1="10" x2="21" y2="10"></line></svg>)svg"},
        {"clock", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>)svg"},
        {"palette", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="13.5" cy="6.5" r="2.5"></circle><circle cx="17.5" cy="10.5" r="2.5"></circle><circle cx="8.5" cy="7.5" r="2.5"></circle><circle cx="6.5" cy="12.5" r="2.5"></circle><path d="M12 2C6.5 2 2 6.5 2 12s4.5 10 10 10c.926 0 1.648-.746 1.648-1.688 0-.437-.18-.835-.437-1.125-.29-.289-.438-.652-.438-1.125a1.64 1.64 0 0 1 1.668-1.668h1.996c3.051 0 5.555-2.503 5.555-5.554C21.965 6.012 17.461 2 12 2z"></path></svg>)svg"},
        {"zap", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"></polygon></svg>)svg"},
        {"monitor", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect><line x1="8" y1="21" x2="16" y2="21"></line><line x1="12" y1="17" x2="12" y2="21"></line></svg>)svg"},
        {"power", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>)svg"},
        {"minimize", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="5" y1="12" x2="19" y2="12"></line></svg>)svg"},
        {"maximize", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect></svg>)svg"},
        {"restore", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="10" height="10" rx="1"></rect><rect x="11" y="3" width="10" height="10" rx="1"></rect></svg>)svg"},
        {"copy", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>)svg"},
        {"pin_vertical", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><path d="M16 9V4h1c.55 0 1-.45 1-1s-.45-1-1-1H7c-.55 0-1 .45-1 1s.45 1 1 1h1v5c0 1.66-1.34 3-3 3v2h5.97v7l1.03 1 1.03-1v-7H19v-2c-1.66 0-3-1.34-3-3z"></path></svg>)svg"},
        {"pin_tilted", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" transform="rotate(45 12 12)"><path d="M16 12V6H8v6l-2 2v2h5v8l1 1 1-1v-8h5v-2l-2-2z"></path></svg>)svg"},
        {"star_filled", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"></polygon></svg>)svg"},
        {"bookmark_filled", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"></path></svg>)svg"},
        {"circle_filled", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><circle cx="12" cy="12" r="8"></circle></svg>)svg"},
        {"edit_clear", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17.5 19H9a2 2 0 0 1-2-2V7a2 2 0 0 1 2-2h8.5L22 12l-4.5 7z"></path><path d="M12 9l4 4"></path><path d="M16 9l-4 4"></path></svg>)svg"},
        {"no_color", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"></line></svg>)svg"},
        {"random_color", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"></path><polyline points="3.27 6.96 12 12.01 20.73 6.96"></polyline><line x1="12" y1="22.08" x2="12" y2="12"></line></svg>)svg"},
        {"screen_picker", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m2 22 1-1h3l9-9"/><path d="M11 8V4.64a1 1 0 0 0-.7-.97L7.64 2.11a1 1 0 0 0-1.27.36L4.13 5.38a1 1 0 0 0 .36 1.27l1.56.91a1 1 0 0 0 .97 0L11 8Z"/><circle cx="17" cy="7" r="5"/></svg>)svg"},
        {"pixel_ruler", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="7" width="20" height="10" rx="2" ry="2" transform="rotate(45 12 12)"/><path d="m8.5 9.5 1 1"/><path d="m11 12 1 1"/><path d="m13.5 14.5 1 1"/><path d="m16 17 1 1"/></svg>)svg"},
        {"screenshot_rect", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/></svg>)svg"},
        {"screenshot_fill", R"svg(<svg viewBox="0 0 24 24" fill="currentColor"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/></svg>)svg"},
        {"screenshot_ellipse", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/></svg>)svg"},
        {"screenshot_arrow", R"svg(<svg viewBox="0 0 24 24" fill="currentColor"><path d="M22 2L11 5L14 8L4 18L6 20L16 10L19 13L22 2Z"/></svg>)svg"},
        {"screenshot_pen", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 19l7-7 3 3-7 7-3-3z"/><path d="M18 13l-1.5-7.5L2 2l3.5 14.5L13 18l5-5z"/></svg>)svg"},
        {"screenshot_marker", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><text x="12" y="16" text-anchor="middle" font-size="12" font-weight="bold" fill="currentColor">1</text></svg>)svg"},
        {"screenshot_mosaic", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="6" height="6"/><rect x="9" y="3" width="6" height="6"/><rect x="15" y="3" width="6" height="6"/><rect x="3" y="9" width="6" height="6"/><rect x="9" y="9" width="6" height="6"/><rect x="15" y="9" width="6" height="6"/><rect x="3" y="15" width="6" height="6"/><rect x="9" y="15" width="6" height="6"/><rect x="15" y="15" width="6" height="6"/></svg>)svg"},
        {"screenshot_confirm", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>)svg"},
        {"screenshot_text", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="4 7 4 4 20 4 20 7"/><line x1="9" y1="20" x2="15" y2="20"/><line x1="12" y1="4" x2="12" y2="20"/></svg>)svg"},
        {"screenshot_line", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="5" y1="19" x2="19" y2="5"></line></svg>)svg"},
        {"screenshot_save", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>)svg"},
        {"screenshot_copy", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>)svg"},
        {"screenshot_close", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>)svg"},
        {"screenshot_eraser", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m7 21-4.3-4.3c-1-1-1-2.5 0-3.4l9.6-9.6c1-1 2.5-1 3.4 0l5.6 5.6c1 1 1 2.5 0 3.4L13 21"/><path d="M22 21H7"/><path d="m5 11 9 9"/></svg>)svg"},
        {"screenshot_pin", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 12V6H8v6l-2 2v2h5v8l1 1 1-1v-8h5v-2l-2-2z"></path></svg>)svg"},
        {"screenshot_ocr", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M7 3H5a2 2 0 0 0-2 2v2M17 3h2a2 2 0 0 1 2 2v2M7 21H5a2 2 0 0 1-2-2v-2M17 21h2a2 2 0 0 0 2-2v-2M8 8h8M8 12h8M8 16h5"/></svg>)svg"},
        {"bold", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><path d="M6 4h8a4 4 0 0 1 4 4 4 4 0 0 1-4 4H6z"></path><path d="M6 12h9a4 4 0 0 1 4 4 4 4 0 0 1-4 4H6z"></path></svg>)svg"},
        {"italic", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><line x1="19" y1="4" x2="10" y2="4"></line><line x1="14" y1="20" x2="5" y2="20"></line><line x1="15" y1="4" x2="9" y2="20"></line></svg>)svg"},
        {"color_wheel", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><path d="M12 2v20M2 12h20M12 2a10 10 0 0 1 7.07 17.07M12 2A10 10 0 0 0 4.93 19.07"/></svg>)svg"},
        {"typesetting", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 6h16M4 12h10M4 18h16"/></svg>)svg"},
        {"find_keyword", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><circle cx="11.5" cy="14.5" r="2.5"></circle><line x1="13.5" y1="16.5" x2="15.5" y2="18.5"></line></svg>)svg"},
        {"swap", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M7 10l5-5 5 5M17 14l-5 5-5-5M12 5v14"/></svg>)svg"},
        {"merge", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M8 6h3a4 4 0 0 1 4 4v2"/><path d="M8 18h3a4 4 0 0 0 4-4v-2"/><path d="M15 12h6"/><polyline points="18 9 21 12 18 15"/></svg>)svg"},
        {"cut", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="6" cy="6" r="3"></circle><circle cx="6" cy="18" r="3"></circle><line x1="20" y1="4" x2="8.12" y2="15.88"></line><line x1="14.47" y1="14.48" x2="20" y2="20"></line><line x1="8.12" y1="8.12" x2="12" y2="12"></line></svg>)svg"},
        {"rotate", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M18 4H10a4 4 0 0 0-4 4v12"/><polyline points="3 17 6 20 9 17"/><path d="M6 20h8a4 4 0 0 0 4-4V4"/><polyline points="21 7 18 4 15 7"/></svg>)svg"},
        {"menu_dots", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="1"/><circle cx="12" cy="5" r="1"/><circle cx="12" cy="19" r="1"/></svg>)svg"},
        {"move", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="5 9 2 12 5 15"/><polyline points="9 5 12 2 15 5"/><polyline points="15 19 12 22 9 19"/><polyline points="19 9 22 12 19 15"/><line x1="2" y1="12" x2="22" y2="12"/><line x1="12" y1="2" x2="12" y2="22"/></svg>)svg"},
        {"help", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"></path><line x1="12" y1="17" x2="12.01" y2="17"></line></svg>)svg"},
        {"scan", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 7V5a2 2 0 0 1 2-2h2"></path><path d="M17 3h2a2 2 0 0 1 2 2v2"></path><path d="M21 17v2a2 2 0 0 1-2 2h-2"></path><path d="M7 21H5a2 2 0 0 1-2-2v-2"></path><line x1="7" y1="12" x2="17" y2="12"></line></svg>)svg"},
        {"camera", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"></path><circle cx="12" cy="13" r="4"></circle></svg>)svg"}
    };
}

#endif // SVGICONS_H
```

## 文件: `src/ui/SvgIcons.h`

```cpp
#ifndef SVGICONS_H
#define SVGICONS_H

#include <QString>
#include <QMap>

namespace SvgIcons {
    inline const QMap<QString, QString> icons = {
        {"text", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" y1="6" x2="20" y2="6"></line><line x1="4" y1="11" x2="14" y2="11"></line><line x1="4" y1="16" x2="20" y2="16"></line><line x1="4" y1="21" x2="14" y2="21"></line></svg>)svg"},
        {"untagged", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z"></path><line x1="7" y1="7" x2="7.01" y2="7"></line><path d="M11 11l4 4m0-4l-4 4" /></svg>)svg"},
        {"tag", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z"></path><line x1="7" y1="7" x2="7.01" y2="7"></line></svg>)svg"},
        {"file", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line><line x1="10" y1="9" x2="8" y2="9"></line></svg>)svg"},
        {"code", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="16 18 22 12 16 6"></polyline><polyline points="8 6 2 12 8 18"></polyline></svg>)svg"},
        {"link", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"></path><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"></path></svg>)svg"},
        {"image", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><circle cx="8.5" cy="8.5" r="1.5"></circle><polyline points="21 15 16 10 5 21"></polyline></svg>)svg"},
        {"branch", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="5" r="3"></circle><path d="M12 8v5"></path><path d="M12 13l-5 4"></path><path d="M12 13l5 4"></path><circle cx="7" cy="19" r="3"></circle><circle cx="17" cy="19" r="3"></circle></svg>)svg"},
        {"category", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="8" y="2" width="8" height="6" rx="1"></rect><path d="M12 8 v3"></path><path d="M12 11 h-6"></path><path d="M12 11 h6"></path><rect x="2" y="13" width="8" height="5" rx="1"></rect><rect x="14" y="13" width="8" height="5" rx="1"></rect><circle cx="12" cy="5" r="1" fill="currentColor"></circle><circle cx="6" cy="15.5" r="1" fill="currentColor"></circle><circle cx="18" cy="15.5" r="1" fill="currentColor"></circle></svg>)svg"},
        {"uncategorized", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M5 8 C5 4 10 4 10 8 C10 11 7 12 7 14" /><circle cx="7" cy="19" r="1" fill="currentColor" stroke="none"/><path d="M14 5 v14" /><path d="M14 6 h3" /> <circle cx="20" cy="6" r="2" /><path d="M14 12 h3" /> <circle cx="20" cy="12" r="2" /><path d="M14 18 h3" /> <circle cx="20" cy="18" r="2" /></svg>)svg"},
        {"trash", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" /><line x1="10" y1="11" x2="10" y2="17" /><line x1="14" y1="11" x2="14" y2="17" /></svg>)svg"},
        {"refresh", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21.5 2v6h-6"></path><path d="M2.5 22v-6h6"></path><path d="M21.5 8A10 10 0 0 0 6 3.5l-3.5 4"></path><path d="M2.5 16A10 10 0 0 0 18 20.5l3.5-4"></path><circle cx="12" cy="12" r="1.5" fill="currentColor" opacity="0.3"></circle></svg>)svg"},
        {"search", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>)svg"},
        {"add", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="12" y1="5" x2="12" y2="19"></line><line x1="5" y1="12" x2="19" y2="12"></line></svg>)svg"},
        {"edit", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"></path><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"></path></svg>)svg"},
        {"bookmark", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"></path></svg>)svg"},
        {"star", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"></polygon></svg>)svg"},
        {"location", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"></path><circle cx="12" cy="10" r="3"></circle></svg>)svg"},
        {"pin", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 12V6H8v6l-2 2v2h5v8l1 1 1-1v-8h5v-2l-2-2z"></path></svg>)svg"},
        {"lock", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>)svg"},
        {"lock_secure", R"svg(<svg viewBox="0 0 24 24" fill="currentColor"><path fill-rule="evenodd" d="M12 2a5 5 0 0 0-5 5v3H6a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8a2 2 0 0 0-2-2h-1V7a5 5 0 0 0-5-5zM9 10V7a3 3 0 0 1 6 0v3H9zm3 4a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3zm-0.75 3h1.5v3h-1.5v-3z" clip-rule="evenodd"/></svg>)svg"},
        // 专门用于"密码生成器"的图标：锁+密码位样式
        {"password_generator", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M16 11V7a4 4 0 0 0-8 0v4" />
            <rect x="3" y="11" width="13" height="10" rx="2" />
            <rect x="11" y="14" width="11" height="7" rx="3.5" />
            <rect x="13.5" y="16.5" width="1.5" height="1.5" fill="currentColor" stroke="none" />
            <rect x="16.25" y="16.5" width="1.5" height="1.5" fill="currentColor" stroke="none" />
            <rect x="19" y="16.5" width="1.5" height="1.5" fill="currentColor" stroke="none" />
        </svg>)svg"},
        {"eye", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>)svg"},
        {"toolbox", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="7" width="20" height="14" rx="2" ry="2"></rect><path d="M6 7V5a2 2 0 0 1 2-2h8a2 2 0 0 1 2 2v2"></path><line x1="12" y1="12" x2="12" y2="16"></line><line x1="8" y1="12" x2="8" y2="16"></line><line x1="16" y1="12" x2="16" y2="16"></line></svg>)svg"},
        {"today", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="4" width="18" height="18" rx="2" ry="2"></rect><line x1="16" y1="2" x2="16" y2="6"></line><line x1="8" y1="2" x2="8" y2="6"></line><line x1="3" y1="10" x2="21" y2="10"></line></svg>)svg"},
        {"all_data", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><ellipse cx="12" cy="5" rx="9" ry="3"></ellipse><path d="M21 12c0 1.66-4 3-9 3s-9-1.34-9-3"></path><path d="M3 5v14c0 1.66 4 3 9 3s9-1.34 9-3V5"></path></svg>)svg"},
        {"sidebar", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><line x1="9" y1="3" x2="9" y2="21"></line></svg>)svg"},
        {"sidebar_right", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><line x1="15" y1="3" x2="15" y2="21"></line></svg>)svg"},
        {"nav_first", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="11 17 6 12 11 7"></polyline><polyline points="18 17 13 12 18 7"></polyline></svg>)svg"},
        {"nav_prev", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 18 9 12 15 6"></polyline></svg>)svg"},
        {"nav_next", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"></polyline></svg>)svg"},
        {"nav_last", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="13 17 18 12 13 7"></polyline><polyline points="6 17 11 12 6 7"></polyline></svg>)svg"},
        {"undo", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 7v6h6"></path><path d="M21 17a9 9 0 0 0-9-9 9 9 0 0 0-6 2.3L3 13"></path></svg>)svg"},
        {"coffee", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 8h1a4 4 0 0 1 0 8h-1"/><path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/><line x1="6" y1="1" x2="6" y2="4"/><line x1="10" y1="1" x2="10" y2="4"/><line x1="14" y1="1" x2="14" y2="4"/></svg>)svg"},
        {"grid", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/><line x1="3" y1="9" x2="21" y2="9"/><line x1="3" y1="15" x2="21" y2="15"/><line x1="9" y1="3" x2="9" y2="21"/><line x1="15" y1="3" x2="15" y2="21"/></svg>)svg"},
        {"book", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20v2H6.5A2.5 2.5 0 0 1 4 19.5z"/><path d="M4 5.5A2.5 2.5 0 0 1 6.5 3H20v2H6.5A2.5 2.5 0 0 1 4 5.5z"/></svg>)svg"},
        {"leaf", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 12c0-4.42-3.58-8-8-8S4 7.58 4 12s3.58 8 8 8 8-3.58 8-8z"/><path d="M12 2a10 10 0 0 0-10 10h20a10 10 0 0 0-10-10z"/></svg>)svg"},
        {"book_open", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>)svg"},
        {"redo", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 7v6h-6"></path><path d="M3 17a9 9 0 0 1 9-9 9 9 0 0 1 6 2.3l3 2.7"></path></svg>)svg"},
        {"list_ul", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="8" y1="6" x2="21" y2="6"></line><line x1="8" y1="12" x2="21" y2="12"></line><line x1="8" y1="18" x2="21" y2="18"></line><line x1="3" y1="6" x2="3.01" y2="6"></line><line x1="3" y1="12" x2="3.01" y2="12"></line><line x1="3" y1="18" x2="3.01" y2="18"></line></svg>)svg"},
        {"list_ol", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="10" y1="6" x2="21" y2="6"></line><line x1="10" y1="12" x2="21" y2="12"></line><line x1="10" y1="18" x2="21" y2="18"></line><path d="M4 6h1v4"></path><path d="M4 10h2"></path><path d="M6 18H4c0-1 2-2 2-3s-1-1.5-2-1"></path></svg>)svg"},
        {"todo", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><path d="M9 12l2 2 4-4"></path></svg>)svg"},
        {"close", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>)svg"},
        {"save", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>)svg"},
        {"filter", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 11l3 3L22 4"/><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"/></svg>)svg"},
        {"select", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 11 12 14 22 4"></polyline><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"></path></svg>)svg"},
        {"grip_diagonal", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="19" cy="19" r="1"></circle><circle cx="19" cy="14" r="1"></circle><circle cx="14" cy="19" r="1"></circle><circle cx="19" cy="9" r="1"></circle><circle cx="14" cy="14" r="1"></circle><circle cx="9" cy="19" r="1"></circle></svg>)svg"},
        {"folder", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path></svg>)svg"},
        {"file_managed", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><path d="M12 18h6v3h-6z" fill="currentColor" stroke="none" /><path d="M12 15h6v1h-6z" fill="currentColor" stroke="none" /></svg>)svg"},
        {"folder_managed", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path><path d="M12 18h8v3h-8z" fill="currentColor" stroke="none" /><path d="M12 15h8v1h-8z" fill="currentColor" stroke="none" /></svg>)svg"},
        {"settings", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>)svg"},
        {"calendar", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="4" width="18" height="18" rx="2" ry="2"></rect><line x1="16" y1="2" x2="16" y2="6"></line><line x1="8" y1="2" x2="8" y2="6"></line><line x1="3" y1="10" x2="21" y2="10"></line></svg>)svg"},
        {"clock", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>)svg"},
        {"palette", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="13.5" cy="6.5" r="2.5"></circle><circle cx="17.5" cy="10.5" r="2.5"></circle><circle cx="8.5" cy="7.5" r="2.5"></circle><circle cx="6.5" cy="12.5" r="2.5"></circle><path d="M12 2C6.5 2 2 6.5 2 12s4.5 10 10 10c.926 0 1.648-.746 1.648-1.688 0-.437-.18-.835-.437-1.125-.29-.289-.438-.652-.438-1.125a1.64 1.64 0 0 1 1.668-1.668h1.996c3.051 0 5.555-2.503 5.555-5.554C21.965 6.012 17.461 2 12 2z"></path></svg>)svg"},
        {"zap", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"></polygon></svg>)svg"},
        {"monitor", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect><line x1="8" y1="21" x2="16" y2="21"></line><line x1="12" y1="17" x2="12" y2="21"></line></svg>)svg"},
        {"power", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>)svg"},
        {"minimize", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="5" y1="12" x2="19" y2="12"></line></svg>)svg"},
        {"maximize", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect></svg>)svg"},
        {"restore", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="10" height="10" rx="1"></rect><rect x="11" y="3" width="10" height="10" rx="1"></rect></svg>)svg"},
        {"copy", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>)svg"},
        {"pin_vertical", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><path d="M16 9V4h1c.55 0 1-.45 1-1s-.45-1-1-1H7c-.55 0-1 .45-1 1s.45 1 1 1h1v5c0 1.66-1.34 3-3 3v2h5.97v7l1.03 1 1.03-1v-7H19v-2c-1.66 0-3-1.34-3-3z"></path></svg>)svg"},
        {"pin_tilted", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" transform="rotate(45 12 12)"><path d="M16 12V6H8v6l-2 2v2h5v8l1 1 1-1v-8h5v-2l-2-2z"></path></svg>)svg"},
        {"star_filled", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"></polygon></svg>)svg"},
        {"bookmark_filled", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"></path></svg>)svg"},
        {"circle_filled", R"svg(<svg viewBox="0 0 24 24" fill="currentColor" stroke="none"><circle cx="12" cy="12" r="8"></circle></svg>)svg"},
        {"edit_clear", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17.5 19H9a2 2 0 0 1-2-2V7a2 2 0 0 1 2-2h8.5L22 12l-4.5 7z"></path><path d="M12 9l4 4"></path><path d="M16 9l-4 4"></path></svg>)svg"},
        {"no_color", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"></line></svg>)svg"},
        {"random_color", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"></path><polyline points="3.27 6.96 12 12.01 20.73 6.96"></polyline><line x1="12" y1="22.08" x2="12" y2="12"></line></svg>)svg"},
        {"screen_picker", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m18 2 4 4"/><path d="m17 7 3-3"/><path d="M19 9 8.7 19.3c-1 1-2.5 1-3.4 0l-.6-.6c-1-1-1-2.5 0-3.4L15 5"/><path d="m9 11 4 4"/><path d="m5 19-3 3"/><path d="m14 4 6 6"/></svg>)svg"},
        {"pixel_ruler", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="7" width="20" height="10" rx="2" ry="2" transform="rotate(45 12 12)"/><path d="m8.5 9.5 1 1"/><path d="m11 12 1 1"/><path d="m13.5 14.5 1 1"/><path d="m16 17 1 1"/></svg>)svg"},
        {"screenshot_rect", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/></svg>)svg"},
        {"screenshot_fill", R"svg(<svg viewBox="0 0 24 24" fill="currentColor"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/></svg>)svg"},
        {"screenshot_ellipse", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/></svg>)svg"},
        {"screenshot_arrow", R"svg(<svg viewBox="0 0 24 24" fill="currentColor"><path d="M22 2L11 5L14 8L4 18L6 20L16 10L19 13L22 2Z"/></svg>)svg"},
        {"screenshot_pen", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 19l7-7 3 3-7 7-3-3z"/><path d="M18 13l-1.5-7.5L2 2l3.5 14.5L13 18l5-5z"/></svg>)svg"},
        {"screenshot_marker", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><text x="12" y="16" text-anchor="middle" font-size="12" font-weight="bold" fill="currentColor">1</text></svg>)svg"},
        {"screenshot_mosaic", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="6" height="6"/><rect x="9" y="3" width="6" height="6"/><rect x="15" y="3" width="6" height="6"/><rect x="3" y="9" width="6" height="6"/><rect x="9" y="9" width="6" height="6"/><rect x="15" y="9" width="6" height="6"/><rect x="3" y="15" width="6" height="6"/><rect x="9" y="15" width="6" height="6"/><rect x="15" y="15" width="6" height="6"/></svg>)svg"},
        {"screenshot_confirm", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>)svg"},
        {"screenshot_text", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="4 7 4 4 20 4 20 7"/><line x1="9" y1="20" x2="15" y2="20"/><line x1="12" y1="4" x2="12" y2="20"/></svg>)svg"},
        {"screenshot_line", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="5" y1="19" x2="19" y2="5"></line></svg>)svg"},
        {"screenshot_save", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>)svg"},
        {"screenshot_copy", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>)svg"},
        {"screenshot_close", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>)svg"},
        {"screenshot_eraser", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m7 21-4.3-4.3c-1-1-1-2.5 0-3.4l9.6-9.6c1-1 2.5-1 3.4 0l5.6 5.6c1 1 1 2.5 0 3.4L13 21"/><path d="M22 21H7"/><path d="m5 11 9 9"/></svg>)svg"},
        {"screenshot_pin", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 12V6H8v6l-2 2v2h5v8l1 1 1-1v-8h5v-2l-2-2z"></path></svg>)svg"},
        {"screenshot_ocr", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M7 3H5a2 2 0 0 0-2 2v2M17 3h2a2 2 0 0 1 2 2v2M7 21H5a2 2 0 0 1-2-2v-2M17 21h2a2 2 0 0 0 2-2v-2M8 8h8M8 12h8M8 16h5"/></svg>)svg"},
        {"bold", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><path d="M6 4h8a4 4 0 0 1 4 4 4 4 0 0 1-4 4H6z"></path><path d="M6 12h9a4 4 0 0 1 4 4 4 4 0 0 1-4 4H6z"></path></svg>)svg"},
        {"italic", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><line x1="19" y1="4" x2="10" y2="4"></line><line x1="14" y1="20" x2="5" y2="20"></line><line x1="15" y1="4" x2="9" y2="20"></line></svg>)svg"},
        {"color_wheel", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><path d="M12 2v20M2 12h20M12 2a10 10 0 0 1 7.07 17.07M12 2A10 10 0 0 0 4.93 19.07"/></svg>)svg"},
        {"typesetting", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 6h16M4 12h10M4 18h16"/></svg>)svg"},
        {"find_keyword", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><circle cx="11.5" cy="14.5" r="2.5"></circle><line x1="13.5" y1="16.5" x2="15.5" y2="18.5"></line></svg>)svg"},
        {"swap", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M7 10l5-5 5 5M17 14l-5 5-5-5M12 5v14"/></svg>)svg"},
        {"merge", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 21V10"/><path d="M12 10l-4-4"/><path d="M12 10l4-4"/><path d="M8 6V3"/><path d="M16 6V3"/></svg>)svg"},
        {"cut", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="6" cy="6" r="3"></circle><circle cx="6" cy="18" r="3"></circle><line x1="20" y1="4" x2="8.12" y2="15.88"></line><line x1="14.47" y1="14.48" x2="20" y2="20"></line><line x1="8.12" y1="8.12" x2="12" y2="12"></line></svg>)svg"},
        {"rotate", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M18 4H10a4 4 0 0 0-4 4v12"/><polyline points="3 17 6 20 9 17"/><path d="M6 20h8a4 4 0 0 0 4-4V4"/><polyline points="21 7 18 4 15 7"/></svg>)svg"},
        {"menu_dots", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="1"/><circle cx="12" cy="5" r="1"/><circle cx="12" cy="19" r="1"/></svg>)svg"},
        {"move", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="5 9 2 12 5 15"/><polyline points="9 5 12 2 15 5"/><polyline points="15 19 12 22 9 19"/><polyline points="19 9 22 12 19 15"/><line x1="2" y1="12" x2="22" y2="12"/><line x1="12" y1="2" x2="12" y2="22"/></svg>)svg"},
        {"help", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"></path><line x1="12" y1="17" x2="12.01" y2="17"></line></svg>)svg"},
        {"scan", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 7V5a2 2 0 0 1 2-2h2"></path><path d="M17 3h2a2 2 0 0 1 2 2v2"></path><path d="M21 17v2a2 2 0 0 1-2 2h-2"></path><path d="M7 21H5a2 2 0 0 1-2-2v-2"></path><line x1="7" y1="12" x2="17" y2="12"></line></svg>)svg"},
        {"camera", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"></path><circle cx="12" cy="13" r="4"></circle></svg>)svg"},
        {"ball_on", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"></circle><path d="M12 8v4l3 2"></path></svg>)svg"},
        {"ball_off", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"></circle><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"></line></svg>)svg"},
        {"paint_bucket", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m19 11-8-8-8.6 8.6a2 2 0 0 0 0 2.8l5.2 5.2c.8.8 2 .8 2.8 0L19 11Z"></path><path d="m5 2 5 5"></path><path d="M2 13h15"></path><path d="M22 20a2 2 0 1 1-4 0c0-1.6 1.7-2.4 2-4 .3-1.6 2-2.4 2-4Z" fill="currentColor" fill-opacity="0.3"></path></svg>)svg"}
    };
}

#endif // SVGICONS_H
```

## 文件: `src/ui/SystemTray.cpp`

```cpp
#include "SystemTray.h"
#include "StringUtils.h"

#include "IconHelper.h"
#include "FloatingBall.h"
#include <QApplication>
#include <QIcon>
#include <QStyle>

SystemTray::SystemTray(QObject* parent) : QObject(parent) {
    m_trayIcon = new QSystemTrayIcon(this);
    
    // 复刻 Python 版：使用渲染的悬浮球作为托盘图标
    m_trayIcon->setIcon(FloatingBall::generateBallIcon());
    m_trayIcon->setToolTip("搜索工具");

    m_menu = new QMenu();
    m_menu->setStyleSheet(
        "QMenu { background-color: #2D2D2D; color: #EEE; border: 1px solid #444; padding: 4px; } "
        /* 10px 间距规范：padding-left 10px + icon margin-left 6px */
        "QMenu::item { padding: 6px 10px 6px 10px; border-radius: 3px; } "
        "QMenu::icon { margin-left: 6px; } "
        "QMenu::item:selected { background-color: #4a90e2; color: white; }"
    );
    
    m_menu->addAction(IconHelper::getIcon("monitor", "#aaaaaa", 18), "显示主界面", this, &SystemTray::showMainWindow);
    m_menu->addAction(IconHelper::getIcon("zap", "#aaaaaa", 18), "显示快速笔记", this, &SystemTray::showQuickWindow);
    
    m_ballAction = new QAction("隐藏悬浮球", this);
    m_ballAction->setIcon(IconHelper::getIcon("ball_off", "#aaaaaa", 18));
    connect(m_ballAction, &QAction::triggered, this, [this](){
        bool willBeVisible = (m_ballAction->text() == "显示悬浮球");
        emit toggleFloatingBall(willBeVisible);
    });
    m_menu->addAction(m_ballAction);

    m_menu->addAction(IconHelper::getIcon("help", "#aaaaaa", 18), "使用说明", this, &SystemTray::showHelpRequested);
    m_menu->addAction(IconHelper::getIcon("settings", "#aaaaaa", 18), "设置", this, &SystemTray::showSettings);
    m_menu->addSeparator();
    m_menu->addAction(IconHelper::getIcon("power", "#aaaaaa", 18), "退出程序", this, &SystemTray::quitApp);

    m_trayIcon->setContextMenu(m_menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::Trigger) {
            emit showQuickWindow();
        }
    });
}

void SystemTray::show() {
    m_trayIcon->show();
}

void SystemTray::updateBallAction(bool visible) {
    if (visible) {
        m_ballAction->setText("隐藏悬浮球");
        m_ballAction->setIcon(IconHelper::getIcon("ball_off", "#aaaaaa", 18));
    } else {
        m_ballAction->setText("显示悬浮球");
        m_ballAction->setIcon(IconHelper::getIcon("ball_on", "#aaaaaa", 18));
    }
}
```

## 文件: `src/windows/SystemTray.cpp`

```cpp
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
```

## 文件: `src/ui/SystemTray.h`

```cpp
#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QSystemTrayIcon>
#include <QMenu>
#include <QObject>

class SystemTray : public QObject {
    Q_OBJECT
public:
    explicit SystemTray(QObject* parent = nullptr);
    void show();

signals:
    void showMainWindow();
    void showQuickWindow();
    void showHelpRequested();
    void showSettings();
    void quitApp();
    void toggleFloatingBall(bool visible);

public slots:
    void updateBallAction(bool visible);

private:
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_menu;
    QAction* m_ballAction;
};

#endif // SYSTEMTRAY_H
```

## 文件: `src/windows/SystemTray.h`

```cpp
#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QSystemTrayIcon>
#include <QMenu>
#include <QObject>

class SystemTray : public QObject {
    Q_OBJECT
public:
    explicit SystemTray(QObject* parent = nullptr);
    void show();

signals:
    void showWindow();
    void quitApp();

private:
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_menu;
};

#endif // SYSTEMTRAY_H
```

## 文件: `src/ui/Toolbox.cpp`

```cpp
#include "Toolbox.h"
#include "IconHelper.h"
#include "StringUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QToolTip>
#include <QSettings>
#include <QCheckBox>
#include <QDialog>
#include <QWindow>

Toolbox::Toolbox(QWidget* parent) : FramelessDialog("搜索工具箱", parent) {
    setObjectName("SearchTool_ToolboxLauncher");
    
    // [CRITICAL] 强制开启非活动窗口的 ToolTip 显示。
    setAttribute(Qt::WA_AlwaysShowToolTips);

    // 设置为工具窗口：任务栏不显示，且置顶
    setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowStaysOnTopHint);

    // 关键修复：强制注入 ToolTip 样式。
    // 在 Windows 平台下，Qt::Tool 窗口的子控件在弹出 ToolTip 时往往无法正确继承全局 QSS。
    this->setStyleSheet(StringUtils::getToolTipStyle());
    
    // 允许通过拉伸边缘来调整大小
    setMinimumSize(40, 40);

    // 修改工具箱圆角为 6px
    QWidget* container = findChild<QWidget*>("DialogContainer");
    if (container) {
        container->setStyleSheet(container->styleSheet().replace("border-radius: 12px;", "border-radius: 6px;"));
    }

    initUI();
    loadSettings();
    updateLayout(m_orientation);
}

Toolbox::~Toolbox() {
    saveSettings();
}

void Toolbox::hideEvent(QHideEvent* event) {
    saveSettings();
    FramelessDialog::hideEvent(event);
}

void Toolbox::initUI() {
    // 隐藏默认标题文字，因为我们要把图标放上去
    m_titleLabel->hide();

    // 置顶按钮在工具箱中永久隐藏
    if (m_btnPin) m_btnPin->hide();

    // 将最小化按钮改为移动手柄
    if (m_minBtn) {
        // 仅断开与基类的连接，避免使用通配符 disconnect() 触发 destroyed 信号警告
        m_minBtn->disconnect(this); 
        m_minBtn->setIcon(IconHelper::getIcon("move", "#888888"));
        m_minBtn->setToolTip(StringUtils::wrapToolTip("按住移动"));
        m_minBtn->setCursor(Qt::SizeAllCursor);
        // 保留 Hover 背景提供视觉反馈
        m_minBtn->setStyleSheet(StringUtils::getToolTipStyle() + 
                             "QPushButton { background: transparent; border: none; border-radius: 4px; } "
                             "QPushButton:hover { background-color: rgba(255, 255, 255, 0.1); }");
        
        // 安装事件过滤器以实现拖拽
        m_minBtn->installEventFilter(this);
    }
    
    // 清空内容区原有边距
    m_contentArea->layout() ? delete m_contentArea->layout() : (void)0;

    // 创建按钮列表
    auto addTool = [&](const QString& id, const QString& tip, const QString& icon, const QString& color, auto signal) {
        ToolInfo info;
        info.id = id;
        info.tip = tip;
        info.icon = icon;
        info.color = color;
        info.callback = [this, signal]() { emit (this->*signal)(); };
        info.btn = createToolButton(tip, icon, color);
        connect(info.btn, &QPushButton::clicked, this, info.callback);
        m_toolInfos.append(info);
    };

    addTool("time", "时间输出", "clock", "#1abc9c", &Toolbox::showTimePasteRequested);
    addTool("password", "密码生成器", "password_generator", "#3498db", &Toolbox::showPasswordGeneratorRequested);
    addTool("ocr", "识别记录", "text", "#4a90e2", &Toolbox::showOCRRequested);
    addTool("immediate_ocr", "文字识别", "screenshot_ocr", "#3498db", &Toolbox::startOCRRequested);
    addTool("tag", "标签管理", "tag", "#f1c40f", &Toolbox::showTagManagerRequested);
    addTool("file_storage", "存储文件", "file_managed", "#e67e22", &Toolbox::showFileStorageRequested);
    addTool("file_search", "搜索文件", "search", "#95a5a6", &Toolbox::showFileSearchRequested);
    addTool("keyword_search", "搜索关键字", "find_keyword", "#3498db", &Toolbox::showKeywordSearchRequested);
    addTool("color_picker", "吸取颜色", "paint_bucket", "#ff6b81", &Toolbox::showColorPickerRequested);
    addTool("immediate_color_picker", "立即取色", "screen_picker", "#ff4757", &Toolbox::startColorPickerRequested);
    addTool("screenshot", "截图", "camera", "#e74c3c", &Toolbox::screenshotRequested);
    addTool("main_window", "主界面", "maximize", "#4FACFE", &Toolbox::showMainWindowRequested);
    addTool("quick_window", "搜索工具", "zap", "#F1C40F", &Toolbox::showQuickWindowRequested);

    m_btnRotate = createToolButton("切换布局", "rotate", "#aaaaaa");
    connect(m_btnRotate, &QPushButton::clicked, this, &Toolbox::toggleOrientation);

    m_btnMenu = createToolButton("配置按钮", "menu_dots", "#aaaaaa");
    connect(m_btnMenu, &QPushButton::clicked, this, &Toolbox::showConfigPanel);
}

void Toolbox::updateLayout(Orientation orientation) {
    m_orientation = orientation;
    
    // 获取控制按钮 (使用基类成员)
    auto* btnPin = m_btnPin;
    auto* minBtn = m_minBtn; // 在工具箱中作为“移动”手柄
    auto* closeBtn = m_closeBtn;

    // 根据方向设置菜单图标（垂直模式下旋转90度变为横向三点）
    if (m_btnMenu) {
        m_btnMenu->setIcon(IconHelper::getIcon("menu_dots", "#aaaaaa"));
        if (orientation == Orientation::Vertical) {
            QPixmap pix = m_btnMenu->icon().pixmap(32, 32);
            QTransform trans;
            trans.rotate(90);
            m_btnMenu->setIcon(QIcon(pix.transformed(trans, Qt::SmoothTransformation)));
        }
    }

    // 寻找标题栏 widget
    QWidget* titleBar = nullptr;
    if (m_mainLayout->count() > 0) {
        titleBar = m_mainLayout->itemAt(0)->widget();
    }
    if (!titleBar) return;

    // 彻底重置标题栏布局与尺寸限制，防止横纵切换冲突导致的 squashed 状态
    titleBar->setMinimumSize(0, 0);
    titleBar->setMaximumSize(16777215, 16777215);
    
    // 移除基类默认的 10px 底部边距，确保尺寸严格受控
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    if (titleBar->layout()) {
        QLayoutItem* item;
        while ((item = titleBar->layout()->takeAt(0)) != nullptr) {
            // 不删除 widget，只移除
        }
        delete titleBar->layout();
    }

    // 统一隐藏内容区，所有按钮都放在标题栏内以便在纵向时能正确拉伸且顺序一致
    m_contentArea->hide();

    int visibleCount = 0;
    for (const auto& info : m_toolInfos) if (info.visible) visibleCount++;

    if (orientation == Orientation::Horizontal) {
        titleBar->setFixedHeight(42);
        titleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        titleBar->setStyleSheet("background-color: transparent; border: none;");
        auto* layout = new QHBoxLayout(titleBar);
        layout->setContentsMargins(8, 0, 8, 0);
        layout->setSpacing(2); // 紧凑间距
        
        // 1. 功能按钮
        for (auto& info : m_toolInfos) {
            if (info.visible) {
                layout->addWidget(info.btn, 0, Qt::AlignVCenter);
                info.btn->show();
            } else {
                info.btn->hide();
            }
        }
        // 2. 旋转与配置按钮
        layout->addWidget(m_btnRotate, 0, Qt::AlignVCenter);
        layout->addWidget(m_btnMenu, 0, Qt::AlignVCenter);
        
        // 4. 系统控制按钮 (统一间距，移除 Stretch)
        if (minBtn) layout->addWidget(minBtn, 0, Qt::AlignVCenter);
        if (closeBtn) layout->addWidget(closeBtn, 0, Qt::AlignVCenter);

        // 确保 m_mainLayout 正确分配空间
        m_mainLayout->setStretchFactor(titleBar, 0);
    } else {
        titleBar->setFixedWidth(42);
        titleBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        titleBar->setStyleSheet("background-color: transparent; border: none;");
        auto* layout = new QVBoxLayout(titleBar);
        layout->setContentsMargins(0, 8, 0, 8);
        layout->setSpacing(2); // 紧凑间距
        layout->setAlignment(Qt::AlignHCenter);

        // 垂直模式下，顺序完全反转：系统按钮在最上方
        if (closeBtn) layout->addWidget(closeBtn, 0, Qt::AlignHCenter);
        if (minBtn) layout->addWidget(minBtn, 0, Qt::AlignHCenter);
        // 置顶按钮在垂直模式也隐藏

        // 旋转与配置按钮 (反转顺序，移除 Stretch 实现统一间距)
        layout->addWidget(m_btnMenu, 0, Qt::AlignHCenter);
        layout->addWidget(m_btnRotate, 0, Qt::AlignHCenter);

        // 功能工具按钮 (反转顺序)
        for (int i = m_toolInfos.size() - 1; i >= 0; --i) {
            auto& info = m_toolInfos[i];
            if (info.visible) {
                layout->addWidget(info.btn, 0, Qt::AlignHCenter);
                info.btn->show();
            } else {
                info.btn->hide();
            }
        }

        // 在纵向模式下，让 titleBar 填满整个布局
        m_mainLayout->setStretchFactor(titleBar, 1);
    }

    // 强制触发布局计算与尺寸同步，确保 sizeHint 有效且不触发 Windows 渲染报错
    titleBar->updateGeometry();
    m_mainLayout->activate();
    
    setMinimumSize(0, 0);
    setMaximumSize(16777215, 16777215);

    // 先通过 adjustSize 让窗口系统同步布局，再锁定固定尺寸，防止 UpdateLayeredWindowIndirect 报错
    adjustSize();
    setFixedSize(sizeHint());
    update();
}

void Toolbox::mouseMoveEvent(QMouseEvent* event) {
    FramelessDialog::mouseMoveEvent(event);
    // 这里可以添加吸附预览效果
}

void Toolbox::mouseReleaseEvent(QMouseEvent* event) {
    FramelessDialog::mouseReleaseEvent(event);
    checkSnapping();
}

void Toolbox::moveEvent(QMoveEvent* event) {
    FramelessDialog::moveEvent(event);
    // 仅在窗口可见且非最小化时保存位置，防止启动时的异常坐标或最小化状态被记录
    if (isVisible() && !isMinimized()) {
        saveSettings();
    }
}

bool Toolbox::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_minBtn) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                // 转发给窗口处理拖拽逻辑
                this->mousePressEvent(me);
                return true; // 拦截，不触发按钮点击
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->buttons() & Qt::LeftButton) {
                this->mouseMoveEvent(me);
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            this->mouseReleaseEvent(me);
            return true;
        }
    }
    return FramelessDialog::eventFilter(watched, event);
}

void Toolbox::checkSnapping() {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QRect screenGeom = screen->availableGeometry();
    QRect winGeom = frameGeometry();
    const int threshold = 40;

    int targetX = winGeom.x();
    int targetY = winGeom.y();
    Orientation newOrientation = m_orientation;

    bool snapped = false;

    // 考虑 FramelessDialog 的 15px 外部边距 (用于阴影)
    const int margin = 15;

    // 检查左右边缘
    if (winGeom.left() + margin - screenGeom.left() < threshold) {
        targetX = screenGeom.left() - margin;
        newOrientation = Orientation::Vertical;
        snapped = true;
    } else if (screenGeom.right() - (winGeom.right() - margin) < threshold) {
        targetX = screenGeom.right() - winGeom.width() + margin;
        newOrientation = Orientation::Vertical;
        snapped = true;
    }

    // 检查上下边缘
    if (winGeom.top() + margin - screenGeom.top() < threshold) {
        targetY = screenGeom.top() - margin;
        if (!snapped) newOrientation = Orientation::Horizontal;
        snapped = true;
    } else if (screenGeom.bottom() - (winGeom.bottom() - margin) < threshold) {
        targetY = screenGeom.bottom() - winGeom.height() + margin;
        if (!snapped) newOrientation = Orientation::Horizontal;
        snapped = true;
    }

    if (snapped) {
        if (newOrientation != m_orientation) {
            updateLayout(newOrientation);
            adjustSize(); // 确保获取更新布局后的最新尺寸
            // 切换布局后再次校验边界，防止超出屏幕 (针对 Requirement 4)
            QRect newWinGeom = frameGeometry();
            if (targetX + newWinGeom.width() - margin > screenGeom.right()) {
                targetX = screenGeom.right() - newWinGeom.width() + margin;
            }
            if (targetY + newWinGeom.height() - margin > screenGeom.bottom()) {
                targetY = screenGeom.bottom() - newWinGeom.height() + margin;
            }
        }
        move(targetX, targetY);
        saveSettings(); // 吸附后显式保存，确保位置被记录
    }
}

void Toolbox::toggleOrientation() {
    Orientation next = (m_orientation == Orientation::Horizontal) ? Orientation::Vertical : Orientation::Horizontal;
    updateLayout(next);
    // 旋转后立即触发吸附与边界检测，防止因高度/宽度增加而溢出屏幕
    checkSnapping();
    saveSettings();
}

void Toolbox::showConfigPanel() {
    auto* panel = new QDialog(this, Qt::Popup | Qt::FramelessWindowHint);
    panel->setAttribute(Qt::WA_TranslucentBackground, true);
    
    auto* mainLayout = new QVBoxLayout(panel);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 引入背景容器 QFrame，彻底解决圆角处直角溢出的问题
    auto* bgFrame = new QFrame(panel);
    bgFrame->setObjectName("ConfigBgFrame");
    bgFrame->setAttribute(Qt::WA_StyledBackground, true);
    
    // 移除 500 像素硬编码宽度，改回自适应内容宽度
    panel->setMinimumWidth(150);

    bgFrame->setStyleSheet(
        "#ConfigBgFrame { background-color: #252526; border: 1px solid #444; border-radius: 10px; }"
        "QLabel { color: #888; border: none; font-size: 11px; font-weight: bold; padding: 2px 5px; background: transparent; }"
        "QCheckBox { background-color: #333336; color: #bbb; border: 1px solid #444; font-size: 11px; padding: 4px 15px; margin: 2px 0px; border-radius: 12px; spacing: 8px; }"
        "QCheckBox:hover { background-color: #404044; color: #fff; border-color: #555; }"
        "QCheckBox::indicator { width: 0px; height: 0px; } " // 胶囊样式下隐藏复选框勾选图标
        "QCheckBox:checked { background-color: rgba(0, 122, 204, 0.3); color: #fff; font-weight: bold; border-color: #007ACC; }"
        "QCheckBox:checked:hover { background-color: rgba(0, 122, 204, 0.4); border-color: #0098FF; }"
    );

    auto* contentLayout = new QVBoxLayout(bgFrame);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(6);

    mainLayout->addWidget(bgFrame);

    auto* titleLabel = new QLabel("显示/隐藏功能按钮");
    contentLayout->addWidget(titleLabel);

    for (int i = 0; i < m_toolInfos.size(); ++i) {
        auto* cb = new QCheckBox(m_toolInfos[i].tip);
        cb->setIcon(IconHelper::getIcon(m_toolInfos[i].icon, m_toolInfos[i].color));
        cb->setIconSize(QSize(18, 18));
        cb->setCursor(Qt::PointingHandCursor);
        cb->setChecked(m_toolInfos[i].visible);
        connect(cb, &QCheckBox::toggled, this, [this, i](bool checked) {
            m_toolInfos[i].visible = checked;
            saveSettings();
            updateLayout(m_orientation);
        });
        contentLayout->addWidget(cb);
    }

    panel->adjustSize();

    QPoint pos = m_btnMenu->mapToGlobal(QPoint(0, 0));
    
    // 获取当前屏幕可用区域，确保不超出边界
    QScreen *screen = QGuiApplication::primaryScreen();
    if (this->window() && this->window()->windowHandle()) {
        screen = this->window()->windowHandle()->screen();
    }
    QRect screenGeom = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    int x = pos.x();
    int y = pos.y();

    if (m_orientation == Orientation::Horizontal) {
        // 优先向上弹出
        y = pos.y() - panel->height() - 5;
        if (y < screenGeom.top()) {
            // 空间不足则向下弹出
            y = pos.y() + m_btnMenu->height() + 5;
        }
        // 水平修正，保持在按钮附近
        if (x + panel->width() > screenGeom.right()) {
            x = screenGeom.right() - panel->width() - 5;
        }
    } else {
        // 纵向模式下，向左弹出
        x = pos.x() - panel->width() - 5;
        if (x < screenGeom.left()) {
            // 空间不足则向右弹出
            x = pos.x() + m_btnMenu->width() + 5;
        }
        // 垂直修正
        if (y + panel->height() > screenGeom.bottom()) {
            y = screenGeom.bottom() - panel->height() - 5;
        }
    }

    panel->move(x, y);
    panel->show();
}

void Toolbox::loadSettings() {
    QSettings settings("SearchTool", "Toolbox");
    m_orientation = (Orientation)settings.value("orientation", (int)Orientation::Vertical).toInt();
    
    if (settings.value("isOpen", false).toBool()) {
        show();
    }

    // 恢复位置
    if (settings.contains("pos")) {
        move(settings.value("pos").toPoint());
    } else {
        // 首次运行：默认停靠在屏幕右侧
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect geom = screen->availableGeometry();
            move(geom.right() - 50, geom.center().y() - 150);
        }
    }

    for (auto& info : m_toolInfos) {
        info.visible = settings.value("visible_" + info.id, true).toBool();
    }
}

void Toolbox::saveSettings() {
    QSettings settings("SearchTool", "Toolbox");
    settings.setValue("orientation", (int)m_orientation);
    settings.setValue("isOpen", isVisible());
    
    // 记录最后一次有效位置
    if (isVisible() && !isMinimized()) {
        settings.setValue("pos", pos());
    }
    
    for (const auto& info : m_toolInfos) {
        settings.setValue("visible_" + info.id, info.visible);
    }
}

QPushButton* Toolbox::createToolButton(const QString& tooltip, const QString& iconName, const QString& color) {
    auto* btn = new QPushButton();
    btn->setIcon(IconHelper::getIcon(iconName, color));
    btn->setIconSize(QSize(20, 20));
    btn->setFixedSize(32, 32);
    // 使用简单的 HTML 包装以确保在所有平台上触发 QSS 样式化的富文本渲染
    btn->setToolTip(StringUtils::wrapToolTip(tooltip));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    
    btn->setStyleSheet(StringUtils::getToolTipStyle() + 
        "QPushButton {"
        "  background-color: transparent;"
        "  border: none;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.08);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(255, 255, 255, 0.15);"
        "}"
    );
    
    return btn;
}
```

## 文件: `src/ui/Toolbox.h`

```cpp
#ifndef TOOLBOX_H
#define TOOLBOX_H

#include "FramelessDialog.h"
#include <QPushButton>
#include <QPoint>
#include <QMoveEvent>
#include <QBoxLayout>
#include <functional>

class Toolbox : public FramelessDialog {
    Q_OBJECT
public:
    explicit Toolbox(QWidget* parent = nullptr);
    ~Toolbox();

    enum class Orientation {
        Horizontal,
        Vertical
    };

signals:
    void showMainWindowRequested();
    void showQuickWindowRequested();
    void showTimePasteRequested();
    void showPasswordGeneratorRequested();
    void showOCRRequested();
    void startOCRRequested();
    void showTagManagerRequested();
    void showFileStorageRequested();
    void showFileSearchRequested();
    void showKeywordSearchRequested();
    void showColorPickerRequested();
    void startColorPickerRequested();
    void showHelpRequested();
    void screenshotRequested();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    // 工具箱自身始终置顶且由构造函数控制，跳过基类的通用置顶记忆逻辑
    void loadWindowSettings() override {}
    void saveWindowSettings() override {}

private slots:
    void toggleOrientation();
    void showConfigPanel();

private:
    void initUI();
    void updateLayout(Orientation orientation);
    void checkSnapping();
    QPushButton* createToolButton(const QString& tooltip, const QString& iconName, const QString& color);
    void loadSettings();
    void saveSettings();

    Orientation m_orientation = Orientation::Vertical;
    
    struct ToolInfo {
        QString id;
        QString tip;
        QString icon;
        QString color;
        std::function<void()> callback;
        QPushButton* btn = nullptr;
        bool visible = true;
    };
    QList<ToolInfo> m_toolInfos;

    QPushButton* m_btnRotate = nullptr;
    QPushButton* m_btnMenu = nullptr;
};

#endif // TOOLBOX_H
```

