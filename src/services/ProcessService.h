#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

struct ProcessResult {
    bool success{false};
    int exitCode{-1};
    QString stdoutOutput;
    QString stderrOutput;
    QString errorMessage;
};

class ProcessService : public QObject
{
    Q_OBJECT

public:
    static ProcessService& instance();

    ProcessResult run(const QString &command,
                      const QStringList &arguments = QStringList(),
                      int timeoutMs = 5000,
                      const QString &workingDir = QString());

    void runAsync(const QString &command,
                  const QStringList &arguments,
                  std::function<void(const ProcessResult &result)> callback,
                  int timeoutMs = 15000,
                  const QString &workingDir = QString());

    bool launchDetached(const QString &command,
                        const QStringList &arguments = QStringList(),
                        const QString &workingDir = QString());

private:
    explicit ProcessService(QObject *parent = nullptr);
};
