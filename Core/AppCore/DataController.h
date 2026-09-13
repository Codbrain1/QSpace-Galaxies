#pragma once
#include "Common/Enums/IOEnums.h"
#include "Visualize/Layers/Layer.h"
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <quuid.h>
#include "Enums/CoreEnums.h"
#include "IO/ReadResult.h"
#include "Physics/Math/MetaDataCalculating.h"
#include "Structures/SessionStructures.h"

// Forward declarations в пространствах имен проекта
namespace QSpace::Core {
class DataManager;
class ObjectRegistry;
class LayerManager;
class ViewManager;
class DataNode;
class Snapshot;
} // namespace QSpace::Core

namespace QSpace::Core::Controllers {

// ---------------------------------------------------------
// @SECTION: загрузка данных
// ---------------------------------------------------------
class DataController : public QObject {
    Q_OBJECT
  public:
    explicit DataController(Core::DataManager*    dataManager,
                            Core::ObjectRegistry* objectRegistry,
                            Core::LayerManager*   layerManager,
                            Core::ViewManager*    viewManager,
                            QObject*              parent = nullptr);

    void initialize();

    // TODO: версионирование для чтения данных заменить на использование заранее подготовленных схем
    // чтения (добавить пользователю возможность самостоятельно создавать схемы для чтения файлов)

    //  --- Импорт и загрузка данных ---

    void importFiles(const QStringList&          paths,
                     IO::ModelingProgrammVersion version = IO::ModelingProgrammVersion::V2,
                     const QUuid&                targetExperimentId = QUuid());

    void
    importExperiment(const QString&              experimentPath,
                     IO::ModelingProgrammVersion version = IO::ModelingProgrammVersion::V2); // TODO

    void importExperiment(const QStringList&          filePaths,
                          const QString&              experimentName,
                          IO::ModelingProgrammVersion version = IO::ModelingProgrammVersion::V2);

    std::optional<QUuid> getExperimentIdByNodePath(const QString& nodePath) const;
    std::optional<QUuid> getNodeIdByFilePath(const QString& filePath) const;

    // --- Управление узлами данных (Nodes) ---
    void                                      createLayerForNode(const QUuid& nodeId);
    std::shared_ptr<QSpace::Core::DataNode>   getNodeById(const QUuid& nodeId);
    std::shared_ptr<Visualize::Layers::Layer> getLayerById(const QUuid& layerId);

    // Метод, который вызовет ProjectController при открытии проекта
    void prepareNodesForRestoration(const QMap<QString, Session::DataNodeState>& restoringNodes);

    QList<std::shared_ptr<QSpace::Core::Experiment>> getExperiments() const;


  signals:
    void sceneUpdateRequested();
    void markSessionDirty(); // Сигнал для ProjectController
    void nodeDataLoaded(const QUuid& nodeId);

  public slots:
    void removeNodeObject(const QUuid& id);
    void removeLayer(const QUuid& id);

    // при выборе ноды в плоском режиме
    void onNodeSelectionActivated(const QUuid& nodeId);

    void onRequestDataLoad(const QUuid& nodeId);


  private slots:
    void handleFileReady(const QUuid& taskId, IO::ReadResult result);

  private:
    struct TaskInfo {
        QUuid targetNodeId;   // Будет пустым для первичного импорта пакета
        int   totalFiles = 1; // Количество файлов в задаче
    };

    Core::DataManager* m_dataManager;

    Core::ObjectRegistry* m_objectRegistry;
    Core::LayerManager*   m_layerManager;
    Core::ViewManager*    m_viewManager;
    QMap<QUuid, QUuid>    m_taskToExperiment;

    QMap<QUuid, TaskInfo>                 m_activeTasks;
    QSet<QUuid>                           m_loadingNodes;
    QMap<QString, Session::DataNodeState> m_restoringNodes;

    bool m_autoGrouping = true;

    QString                         extractGroupName(const QString& filename);
    std::shared_ptr<Core::Snapshot> findOrCreateSnapshot(const QString& groupName);
    void activateSnapshotInternal(const QUuid& snapshotId, bool isPreview);
};
} // namespace QSpace::Core::Controllers