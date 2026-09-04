#include "IconImageProvider.h"
#include <QFileInfo>
#include <QPainter>

IconImageProvider::IconImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    int width = (requestedSize.width() > 0 && requestedSize.width() <= 256) ? requestedSize.width() : 64;
    int height = (requestedSize.height() > 0 && requestedSize.height() <= 256) ? requestedSize.height() : 64;

    if (size) {
        *size = QSize(width, height);
    }

    if (id.isEmpty()) {
        QPixmap empty(width, height);
        empty.fill(Qt::transparent);
        return empty;
    }

    // Check if it's an absolute file path
    if (id.startsWith('/') || id.startsWith("file://")) {
        QString path = id.startsWith("file://") ? id.mid(7) : id;
        QFileInfo fi(path);
        if (fi.exists()) {
            QPixmap pix(path);
            if (!pix.isNull()) {
                return pix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }

    // Check embedded resource icons
    if (id == "mvspotlight" || id == "spotlight-qt") {
        QPixmap pix(":/assets/icons/mvspotlight.svg");
        if (!pix.isNull()) {
            return pix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    // Try freedesktop icon theme
    QIcon icon = QIcon::fromTheme(id);
    if (!icon.isNull()) {
        QPixmap pix = icon.pixmap(width, height);
        if (!pix.isNull()) {
            return pix;
        }
    }

    // Try fallback standard freedesktop icons
    QStringList fallbacks = {"application-x-executable", "dialog-information", "applications-other"};
    for (const QString &fb : fallbacks) {
        QIcon fbIcon = QIcon::fromTheme(fb);
        if (!fbIcon.isNull()) {
            QPixmap pix = fbIcon.pixmap(width, height);
            if (!pix.isNull()) return pix;
        }
    }

    // Placeholder pixmap
    QPixmap placeholder(width, height);
    placeholder.fill(Qt::transparent);
    QPainter painter(&placeholder);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(120, 120, 130, 80));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, width, height, 8, 8);
    return placeholder;
}
