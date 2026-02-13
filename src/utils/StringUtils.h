#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>

class StringUtils {
public:
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