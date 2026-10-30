#include "DataManager.h"
#include "Core/TaskManager/TaskManager.h"
#include <qcontainerfwd.h>
#include <qfileinfo.h>
#include <qfuturewatcher.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <quuid.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkSmartPointer.h>
#include "IO/ReaderFactory.h"
#include <algorithm>
#include <memory>

// TODO: добавить возможность отменить чтение через std::function<bool()> isCanceled = []{ return
// false; }
namespace QSpace::Core {
DataManager::DataManager(TaskManager* taskManager, QObject* parent)
    : QObject(parent), m_taskManager(taskManager) {
}

QUuid DataManager::importDataAsync(const QString&        path,
                                   const IO::ReadScheme& scheme,
                                   IO::ImportRole        role) {
    qint64 fileSize = QFileInfo(path).size();
    auto   priority = getPriority(fileSize);
    QUuid  taskId   = QUuid::createUuid();
    emit   ioStarted(taskId, QString("Import File: " + QFileInfo(path).fileName()), 1);
    auto*  watcher = new QFutureWatcher<IO::ReadResult>(this);
    connect(watcher,
            &QFutureWatcher<IO::ReadResult>::finished,
            this,
            [this, watcher, taskId, role]() {
                IO::ReadResult result = watcher->result();
                if (result.isSuccess()) {
                    emit fileReady(taskId, result, role);
                } else {
                    emit fileReadFailed(taskId, result, role);
                    emit errorOccured(result.errMessage);
                }
                emit ioFinished(taskId, result.isSuccess());
                watcher->deleteLater();
            });

    watcher->setFuture(
        m_taskManager->runIO(priority,
                             [path, scheme, policy = m_global_policy]() -> IO::ReadResult {
                                 auto type   = IO::Utils::getFormat(path);
                                 auto reader = IO::createReader(type);
                                 if (!reader) {
                                     return IO::ReadResult{nullptr,
                                                           path,
                                                           "Unsupported file format",
                                                           0,
                                                           0,
                                                           IO::FileFormat::BIN,
                                                           scheme,
                                                           IO::ReadStatus::InvalidFormat};
                                 }
                                 reader->setPolicy(policy);
                                 auto res = reader->read(path, scheme);
                                 res.path = path;
                                 return res;
                             }));
    return taskId;
}

QUuid DataManager::importBatchDataAsync(const QList<IO::BatchTask>& tasks, IO::ImportRole role) {
    QUuid taskId = QUuid::createUuid();
    if (tasks.isEmpty()) {
        emit errorOccured(QString("Tasks list is empty"));
        emit ioFinished(taskId, false);
        return taskId;
    }

    emit  ioStarted(taskId, QString("Import Batch"), tasks.size());
    auto* watcher = new QFutureWatcher<IO::ReadResult>(this);
    connect(watcher,
            &QFutureWatcher<IO::ReadResult>::resultReadyAt,
            this,
            [this, watcher, taskId, role](int index) {
                IO::ReadResult result = watcher->resultAt(index);
                if (result.isSuccess()) {
                    emit fileReady(taskId, result, role);
                } else {
                    emit fileReadFailed(taskId, result, role);
                    emit errorOccured(result.errMessage);
                }
                emit progressChanged(taskId, watcher->progressValue(), watcher->progressMaximum());
            });
    connect(watcher,
            &QFutureWatcher<IO::ReadResult>::finished,
            this,
            [this, watcher, taskId]() {
                const auto results = watcher->future().results();
                const bool success = !results.isEmpty() && std::all_of(
                    results.cbegin(), results.cend(), [](const IO::ReadResult& result) {
                        return result.isSuccess();
                    });
                emit ioFinished(taskId, success);
                watcher->deleteLater();
            });
    auto policy = m_global_policy;
    if (tasks.size() >= 30) {
        policy = IO::FilePolicy::ForceMapped;
    }
    watcher->setFuture(
        m_taskManager->mapIO(tasks, [policy](const IO::BatchTask& t) -> IO::ReadResult {
            auto type   = IO::Utils::getFormat(t.path);
            auto reader = IO::createReader(type);
            if (!reader)
                return {nullptr,
                        t.path,
                        "Unsupported file format",
                        0,
                        0,
                        IO::FileFormat::BIN,
                        t.scheme,
                        IO::ReadStatus::InvalidFormat};
            reader->setPolicy(policy);

            auto res = reader->read(t.path, t.scheme);
            res.path = t.path;
            return res;
        }));
    return taskId;
}

void DataManager::importBatchIdendicalDataAsync(const QStringList&    paths,
                                                const IO::ReadScheme& scheme,
                                                IO::FileFormat        format,
                                                IO::ImportRole        role) {
    if (paths.isEmpty()) {
        emit errorOccured(QString("Paths list is empty"));
        return;
    }
    auto reader = IO::createReader(format);
    if (!reader) {
        emit errorOccured(QString("Unsupported file format"));
        return;
    }
    QUuid taskId = QUuid::createUuid();
    emit  ioStarted(taskId, "Import Batch", paths.size());

    auto* watcher = new QFutureWatcher<IO::ReadResult>(this);
    connect(watcher,
            &QFutureWatcher<IO::ReadResult>::resultReadyAt,
            this,
            [this, watcher, taskId, role](int index) {
                IO::ReadResult result = watcher->resultAt(index);
                if (result.isSuccess()) {
                    emit fileReady(taskId, result, role);
                } else {
                    emit fileReadFailed(taskId, result, role);
                    emit errorOccured(result.errMessage);
                }
                emit progressChanged(taskId, watcher->progressValue(), watcher->progressMaximum());
            });
    connect(watcher,
            &QFutureWatcher<IO::ReadResult>::finished,
            this,
            [this, watcher, taskId]() {
                const auto results = watcher->future().results();
                const bool success = !results.isEmpty() && std::all_of(
                    results.cbegin(), results.cend(), [](const IO::ReadResult& result) {
                        return result.isSuccess();
                    });
                emit ioFinished(taskId, success);
                watcher->deleteLater();
            });
    const auto policy = m_global_policy;
    watcher->setFuture(
        m_taskManager->mapIO(paths, [format, policy, scheme](const QString& path) -> IO::ReadResult {
            auto reader = IO::createReader(format);
            if (!reader) {
                return {nullptr,
                        path,
                        "Unsupported file format",
                        0,
                        0,
                        format,
                        scheme,
                        IO::ReadStatus::InvalidFormat};
            }
            reader->setPolicy(policy);
            auto result = reader->read(path, scheme);
            result.path = path;
            return result;
        }));
}

TaskPriority DataManager::getPriority(qint64 fileSize) {
    if (fileSize < 64 * 1024 * 1024) {
        return TaskPriority::Urgent;
    } else {
        return TaskPriority::Bulk;
    }
}
} // namespace QSpace::Core