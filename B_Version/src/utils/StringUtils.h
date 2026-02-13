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