#pragma once
#include "Common/Structures/ObjectRegistryStructures.h"
#include "Common/Structures/SessionStructures.h"
#include <QObject>
#include <qobject.h>
#include <qtmetamacros.h>
#include <quuid.h>
#include "objectregistry_export.h"
#include <memory>

namespace QSpace::Core {
class OBJECTREGISTRY_EXPORT ObjectRegistry : public QObject {
    Q_OBJECT
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY nodeAdded)
    Q_PROPERTY(int snapshotCount READ snapshotCount NOTIFY snapshotAdded)
    Q_PROPERTY(int experimentCount READ experimentCount NOTIFY experimentAdded)
    Q_PROPERTY(int cacheCapacity READ cacheCapacity WRITE setCacheCapacity)

  public:
    explicit ObjectRegistry(QObject* parent = nullptr);

    int nodeCount() const {
        return m_nodes.size();
    }

    int snapshotCount() const {
        return m_snapshots.size();
    }

    int experimentCount() const {
        return m_experiments.size();
    }

    int cacheCapacity() const {
        return static_cast<int>(m_cacheCapacity);
    }

    void setCacheCapacity(int capacity) {
        m_cacheCapacity = capacity;
    }

    bool containsExperiment(const QUuid& id) const {
        return m_experiments.contains(id);
    }

    bool containsSnpashot(const QUuid& id) const {
        return m_snapshots.contains(id);
    }

    bool containsDataNode(const QUuid& id) const {
        return m_nodes.contains(id);
    }

    // Регистрация объектов
    void registerNode(std::shared_ptr<DataNode> node);
    void registerNodeToExperiment(std::shared_ptr<DataNode> node, const QUuid& experimentId);
    void registerNodeToSnapshot(std::shared_ptr<DataNode> node, const QUuid& parentSnapshotId);
    void registerSnapshot(std::shared_ptr<Snapshot> snapshot);
    void registerSnapshot(std::shared_ptr<Snapshot> snapshot, const QUuid& experimentId);
    void registerExperiment(std::shared_ptr<Experiment> experiment);

    std::shared_ptr<DataNode>   getNode(const QUuid& id) const;
    std::shared_ptr<DataNode>   getOrLoadNodeData(const QUuid& id);
    std::shared_ptr<Snapshot>   getSnapshot(const QUuid& id) const;
    std::shared_ptr<Experiment> getExperiment(const QUuid& id) const;

    void                        removeObject(const QUuid& id);
    std::shared_ptr<Snapshot>   findSnapshotByName(const QString& name) const;
    std::shared_ptr<Experiment> findExperimentByName(const QString& name) const;


    void updateNodeData(const QUuid& id, vtkSmartPointer<vtkDataSet> dataSet, double timestamp);

    Q_INVOKABLE QList<std::shared_ptr<DataNode>> getAllNodes() {
        return m_nodes.values();
    }

    Q_INVOKABLE QList<std::shared_ptr<Snapshot>> getAllSnapshots() {
        return m_snapshots.values();
    }

    Q_INVOKABLE QList<std::shared_ptr<Experiment>> getAllExperiments() {
        return m_experiments.values();
    }

    Q_INVOKABLE void clear();
  signals:
    void nodeAdded(std::shared_ptr<QSpace::Core::DataNode> node, const QUuid& parentSnapshotId);
    void snapshotAdded(std::shared_ptr<QSpace::Core::Snapshot> container, const QUuid& parentExperimentId);
    void experimentAdded(std::shared_ptr<QSpace::Core::Experiment> experiment);
    void objectRemoved(const QUuid& id);
    void cleared();

    // вызывается, когда данные ноды загрузились в ОЗУ или были выгружены кэшем
    void nodeDataUpdated(const QUuid& id);
    void dataLoadRequested(const QUuid& id);

  private:
    size_t                                  m_cacheCapacity = 30;
    std::list<QUuid>                        m_lruList;
    QMap<QUuid, std::list<QUuid>::iterator> m_lruMap;

    QMap<QUuid, std::shared_ptr<DataNode>>   m_nodes;
    QMap<QUuid, std::shared_ptr<Snapshot>>   m_snapshots;
    QMap<QUuid, std::shared_ptr<Experiment>> m_experiments;
    void                                     touchNodeInMemory(const QUuid& id);
};
} // namespace QSpace::Core