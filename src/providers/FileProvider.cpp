#include "FileProvider.h"
#include "../services/ProcessService.h"
#include "../services/ClipboardService.h"
#include "../services/UsageHistory.h"
#include "../services/ConfigService.h"
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDesktopServices>
#include <QUrl>
#include <QSet>
#include <algorithm>

FileProvider::FileProvider(QObject *parent)
    : SearchProvider(parent)
{
    m_threadPool.setMaxThreadCount(2);
}

FileProvider::~FileProvider()
{
    m_threadPool.waitForDone();
}

QStringList FileProvider::searchDirectories() const
{
    QStringList dirs;
    dirs << QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
         << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
         << QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
         << QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
         << QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
         << QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
         << QDir::homePath() + "/Projects"
         << QDir::homePath() + "/Development"
         << QDir::homePath() + "/Workspace";

    // Read configured workspace path from settings if provided
    QString customWs = ConfigService::instance().getPluginSetting("org.mvspotlight.devtools", "workspace_path", "").toString().trimmed();
    if (!customWs.isEmpty()) {
        if (customWs.startsWith("~/")) {
            customWs = QDir::homePath() + customWs.mid(1);
        } else if (customWs == "~") {
            customWs = QDir::homePath();
        }
        dirs << customWs;
    }

    QStringList validDirs;
    for (const QString &d : dirs) {
        if (!d.isEmpty() && QDir(d).exists()) {
            validDirs.append(QDir::cleanPath(d));
        }
    }
    validDirs.removeDuplicates();
    return validDirs;
}

QString FileProvider::iconForFile(const QString &filePath, bool isDir) const
{
    if (isDir) return "folder";

    static QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension);
    if (!mime.iconName().isEmpty()) {
        return mime.iconName();
    }
    return "text-x-generic";
}

void FileProvider::searchAsync(quint64 requestId, const QString &query)
{
    m_latestRequestId.store(requestId);
    QString q = query.trimmed();

    // Skip very short queries for disk scan
    if (q.length() < 2) {
        emit resultsReady(requestId, {});
        return;
    }

    QStringList dirs = searchDirectories();

    m_threadPool.start([this, requestId, q, dirs]() {
        QList<SearchResult> results;
        QSet<QString> seenPaths;
        QString lowerQuery = q.toLower();

        // 1. Check top-level search directories themselves
        for (const QString &dirPath : dirs) {
            if (m_latestRequestId.load() != requestId) {
                return; // Cancelled
            }

            QFileInfo dirInfo(dirPath);
            QString dirName = dirInfo.fileName();
            QString lowerDirName = dirName.toLower();

            if (lowerDirName.contains(lowerQuery)) {
                double score = 75.0;
                if (lowerDirName == lowerQuery) score = 95.0;
                else if (lowerDirName.startsWith(lowerQuery)) score = 85.0;

                score += UsageHistory::instance().frecencyBoost("file:" + dirInfo.absoluteFilePath());

                SearchResult sr;
                sr.setId("file:" + dirInfo.absoluteFilePath());
                sr.setTitle(dirName);
                sr.setSubtitle(dirInfo.absoluteFilePath());
                sr.setIcon("folder");
                sr.setScore(score);
                sr.setType("Folder");
                sr.setProvider("Files");
                sr.setAction("open");
                sr.setMetadataValue("filePath", dirInfo.absoluteFilePath());
                sr.setMetadataValue("isDir", true);
                sr.setSecondaryActionLabel("Copy Folder Path");
                sr.setSecondaryAction("copy_path");

                results.append(sr);
                seenPaths.insert(dirInfo.absoluteFilePath());
            }
        }

        // 2. Iterate inside directories
        for (const QString &dirPath : dirs) {
            if (m_latestRequestId.load() != requestId) {
                return; // Cancelled
            }

            QDirIterator it(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            int count = 0;
            while (it.hasNext() && count < 200) {
                if (m_latestRequestId.load() != requestId) {
                    return; // Cancelled
                }

                it.next();
                count++;

                QFileInfo fi = it.fileInfo();
                QString filePath = fi.absoluteFilePath();
                if (seenPaths.contains(filePath))
                    continue;

                QString fileName = fi.fileName();
                QString lowerName = fileName.toLower();

                if (lowerName.contains(lowerQuery)) {
                    double score = 55.0;
                    if (lowerName == lowerQuery) score = 90.0;
                    else if (lowerName.startsWith(lowerQuery)) score = 80.0;

                    score += UsageHistory::instance().frecencyBoost("file:" + filePath);

                    SearchResult sr;
                    sr.setId("file:" + filePath);
                    sr.setTitle(fileName);
                    sr.setSubtitle(fi.absolutePath());
                    sr.setIcon(iconForFile(filePath, fi.isDir()));
                    sr.setScore(score);
                    sr.setType(fi.isDir() ? "Folder" : "File");
                    sr.setProvider("Files");
                    sr.setAction("open");
                    sr.setMetadataValue("filePath", filePath);
                    sr.setMetadataValue("isDir", fi.isDir());
                    sr.setSecondaryActionLabel(fi.isDir() ? "Copy Folder Path" : "Copy File Path");
                    sr.setSecondaryAction("copy_path");

                    results.append(sr);
                    seenPaths.insert(filePath);

                    if (results.size() >= 15)
                        break;
                }
            }

            if (results.size() >= 15)
                break;
        }

        if (m_latestRequestId.load() == requestId) {
            std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
                return a.score() > b.score();
            });
            emit resultsReady(requestId, results);
        }
    });
}

bool FileProvider::execute(const SearchResult &result, const QString &action)
{
    QString filePath = result.metadataValue("filePath").toString();
    if (filePath.isEmpty())
        return false;

    if (action == "copy_path") {
        ClipboardService::instance().setText(filePath);
        return true;
    }

    UsageHistory::instance().recordLaunch(result.id());
    bool ok = ProcessService::instance().launchDetached("gio", {"open", filePath});
    if (!ok) {
        ok = ProcessService::instance().launchDetached("xdg-open", {filePath});
    }
    if (!ok) {
        ok = QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
    return ok;
}
