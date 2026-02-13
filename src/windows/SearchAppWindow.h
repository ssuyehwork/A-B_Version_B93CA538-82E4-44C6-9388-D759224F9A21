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
