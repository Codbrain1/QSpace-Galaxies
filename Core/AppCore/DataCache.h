#pragma once

#include "IO/ReadResult.h"
#include <QObject>
#include <QSet>
#include <QMap>
#include <memory>

namespace QSpace::Core {
class DataManager;
class ObjectRegistry;
class FileSchemeRegistry;
}

namespace QSpace::Core::Controllers {

class DataCache final : public QObject {
    Q_OBJECT
public:
    explicit DataCache(Core::DataManager*        dataManager,
                       Core::ObjectRegistry*     objectRegistry,
                       Core::FileSchemeRegistry* fileSchemeRegistry,
                       QObject* parent = nullptr);

    void initialize();
    void setCapacity(int capacity);
    int capacity() const;
    void requestLoad(const QUuid& nodeId);

signals:
    void nodeDataLoaded(const QUuid& nodeId);

private slots:
    void handleFileReady(const QUuid& taskId, const IO::ReadResult& result, IO::ImportRole role);
    void handleIoFinished(const QUuid& taskId, bool success);

private:
    struct LoadingTask {
        QUuid nodeId;
    };

    Core::DataManager*        m_dataManager;
    Core::ObjectRegistry*     m_objectRegistry;
    Core::FileSchemeRegistry* m_fileSchemeRegistry;
    QSet<QUuid> m_loadingNodes;
    QMap<QUuid, LoadingTask> m_tasks;
};

} // namespace QSpace::Core::Controllers
