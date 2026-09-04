#include "ClipboardService.h"
#include <QGuiApplication>
#include <QClipboard>

ClipboardService& ClipboardService::instance()
{
    static ClipboardService s_instance;
    return s_instance;
}

ClipboardService::ClipboardService(QObject *parent)
    : QObject(parent)
{
}

void ClipboardService::setText(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(text, QClipboard::Clipboard);
        clipboard->setText(text, QClipboard::Selection);
    }
}

QString ClipboardService::text() const
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        return clipboard->text(QClipboard::Clipboard);
    }
    return QString();
}
