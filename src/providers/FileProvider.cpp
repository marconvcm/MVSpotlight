#include "FileProvider.h"
#include "../services/ProcessService.h"
#include "../services/ClipboardService.h"
#include "../services/UsageHistory.h"
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMimeDatabase>
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

    QStringList validDirs;
    for (const QString &d : dirs) {
        if (!d.isEmpty() && QDir(d).exists()) {
            validDirs.append(d);
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
        QString lowerQuery = q.toLower();

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
                QString fileName = fi.fileName();
                QString lowerName = fileName.toLower();

                if (lowerName.contains(lowerQuery)) {
                    double score = 55.0;
                    if (lowerName == lowerQuery) score = 90.0;
                    else if (lowerName.startsWith(lowerQuery)) score = 80.0;

                    score += UsageHistory::instance().frecencyBoost("file:" + fi.absoluteFilePath());

                    SearchResult sr;
                    sr.setId("file:" + fi.absoluteFilePath());
                    sr.setTitle(fileName);
                    sr.setSubtitle(fi.absolutePath());
                    sr.setIcon(iconForFile(fi.absoluteFilePath(), fi.isDir()));
                    sr.setScore(score);
                    sr.setType("File");
                    sr.setProvider("Files");
                    sr.setAction("open");
                    sr.setMetadataValue("filePath", fi.absoluteFilePath());
                    sr.setMetadataValue("isDir", fi.isDir());
                    sr.setSecondaryActionLabel("Copy File Path");
                    sr.setSecondaryAction("copy_path");

                    results.append(sr);

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
    return ProcessService::instance().launchDetached("gio", {"open", filePath});
}
