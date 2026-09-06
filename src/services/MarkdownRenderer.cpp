#include "MarkdownRenderer.h"
#include <QTextDocument>

QString MarkdownRenderer::toHtml(const QString &markdown, bool isDark, const QString &accentColor)
{
    if (markdown.trimmed().isEmpty()) {
        return QString();
    }

    QTextDocument doc;
    doc.setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);

    QString html = doc.toHtml();

    // Palette values adapting to Dark / Light Adwaita styling
    QString bgCode = isDark ? "#232328" : "#F0F0F5";
    QString textCode = isDark ? "#E6E6EC" : "#1C1C22";
    QString borderColor = isDark ? "#383842" : "#D4D4DC";
    QString textColor = isDark ? "#F6F6F6" : "#1A1A1E";
    QString quoteColor = isDark ? "#9A9996" : "#77767B";
    QString linkColor = accentColor.isEmpty() ? "#3584E4" : accentColor;
    QString thBg = isDark ? "#2B2B32" : "#EAEAEA";

    QString customCss = QString(
        "<style type=\"text/css\">\n"
        "body { color: %1; font-family: system-ui, -apple-system, sans-serif; font-size: 13px; line-height: 1.55; margin: 0; padding: 0; }\n"
        "p, li { white-space: pre-wrap; line-height: 1.55; margin-top: 5px; margin-bottom: 5px; color: %1; }\n"
        "h1 { font-size: 18px; font-weight: 700; margin-top: 14px; margin-bottom: 6px; color: %1; }\n"
        "h2 { font-size: 16px; font-weight: 700; margin-top: 12px; margin-bottom: 6px; color: %1; }\n"
        "h3 { font-size: 14px; font-weight: 600; margin-top: 10px; margin-bottom: 4px; color: %1; }\n"
        "pre { background-color: %2; color: %3; font-family: 'JetBrains Mono', 'Fira Code', 'DejaVu Sans Mono', monospace; font-size: 12px; padding: 10px 12px; border-radius: 8px; border: 1px solid %4; margin-top: 8px; margin-bottom: 8px; }\n"
        "code { background-color: %2; color: %5; font-family: 'JetBrains Mono', 'Fira Code', monospace; font-size: 12px; padding: 2px 5px; border-radius: 4px; }\n"
        "blockquote { border-left: 3px solid %5; margin-left: 0px; padding-left: 12px; color: %6; }\n"
        "a { color: %7; text-decoration: none; font-weight: 500; }\n"
        "hr { border: 0; border-top: 1px solid %4; height: 1px; margin-top: 12px; margin-bottom: 12px; }\n"
        "table { border-collapse: collapse; margin-top: 8px; margin-bottom: 8px; width: 100%%; }\n"
        "td, th { border: 1px solid %4; padding: 6px 10px; font-size: 12px; }\n"
        "th { background-color: %8; font-weight: 600; }\n"
        "</style>\n"
    ).arg(textColor, bgCode, textCode, borderColor, accentColor, quoteColor, linkColor, thBg);

    // Insert CSS into head
    int headPos = html.indexOf("</head>");
    if (headPos != -1) {
        html.insert(headPos, customCss);
    } else {
        html.prepend(customCss);
    }

    return html;
}
