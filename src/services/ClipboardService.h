#pragma once

#include <QObject>
#include <QString>

class ClipboardService : public QObject
{
    Q_OBJECT

public:
    static ClipboardService& instance();

    Q_INVOKABLE void setText(const QString &text);
    Q_INVOKABLE QString text() const;

private:
    explicit ClipboardService(QObject *parent = nullptr);
};
