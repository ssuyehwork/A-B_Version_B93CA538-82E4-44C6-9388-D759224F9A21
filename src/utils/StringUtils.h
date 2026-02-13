#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>
#include <QTextDocument>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class StringUtils {
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