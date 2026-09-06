#pragma once

#include <QString>

class MarkdownRenderer
{
public:
    static QString toHtml(const QString &markdown, bool isDark = true, const QString &accentColor = "#8A2BE2");
};
