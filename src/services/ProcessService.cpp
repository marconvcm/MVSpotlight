#include "ProcessService.h"
#include <QProcess>
#include <QTimer>
#include <QDebug>

ProcessService& ProcessService::instance()
{
    static ProcessService s_instance;
    return s_instance;
}

ProcessService::ProcessService(QObject *parent)
    : QObject(parent)
{
}

ProcessResult ProcessService::run(const QString &command,
                                  const QStringList &arguments,
                                  int timeoutMs,
                                  const QString &workingDir)
{
    ProcessResult res;
    QProcess proc;
    if (!workingDir.isEmpty())
        proc.setWorkingDirectory(workingDir);

    proc.start(command, arguments);
    if (!proc.waitForStarted(timeoutMs > 0 ? timeoutMs : 5000)) {
        res.success = false;
        res.errorMessage = proc.errorString();
        return res;
    }

    if (!proc.waitForFinished(timeoutMs > 0 ? timeoutMs : 5000)) {
        proc.kill();
        proc.waitForFinished(500);
        res.success = false;
        res.errorMessage = "Process timed out";
        return res;
    }

    res.exitCode = proc.exitCode();
    res.stdoutOutput = QString::fromUtf8(proc.readAllStandardOutput());
    res.stderrOutput = QString::fromUtf8(proc.readAllStandardError());
    res.success = (proc.exitStatus() == QProcess::NormalExit && res.exitCode == 0);
    return res;
}

void ProcessService::runAsync(const QString &command,
                              const QStringList &arguments,
                              std::function<void(const ProcessResult &result)> callback,
                              int timeoutMs,
                              const QString &workingDir)
{
    QProcess *proc = new QProcess();
    if (!workingDir.isEmpty())
        proc->setWorkingDirectory(workingDir);

    QTimer *timer = new QTimer(proc);
    timer->setSingleShot(true);

    auto finish = [proc, timer, callback](bool timedOut) {
        timer->stop();
        ProcessResult res;
        if (timedOut) {
            proc->kill();
            proc->waitForFinished(500);
            res.success = false;
            res.errorMessage = "Process timed out";
        } else {
            res.exitCode = proc->exitCode();
            res.stdoutOutput = QString::fromUtf8(proc->readAllStandardOutput());
            res.stderrOutput = QString::fromUtf8(proc->readAllStandardError());
            res.success = (proc->exitStatus() == QProcess::NormalExit && res.exitCode == 0);
        }
        if (callback)
            callback(res);
        proc->deleteLater();
    };

    QObject::connect(timer, &QTimer::timeout, [finish]() {
        finish(true);
    });

    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [finish](int, QProcess::ExitStatus) {
        finish(false);
    });

    QObject::connect(proc, &QProcess::errorOccurred, [proc, timer, callback](QProcess::ProcessError) {
        timer->stop();
        ProcessResult res;
        res.success = false;
        res.errorMessage = proc->errorString();
        if (callback)
            callback(res);
        proc->deleteLater();
    });

    proc->start(command, arguments);
    if (timeoutMs > 0)
        timer->start(timeoutMs);
}

bool ProcessService::launchDetached(const QString &command,
                                    const QStringList &arguments,
                                    const QString &workingDir)
{
    return QProcess::startDetached(command, arguments, workingDir);
}
