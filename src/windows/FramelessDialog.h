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
