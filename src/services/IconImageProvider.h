#pragma once

#include <QQuickImageProvider>
#include <QIcon>
#include <QPixmap>

class IconImageProvider : public QQuickImageProvider
{
public:
    IconImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
