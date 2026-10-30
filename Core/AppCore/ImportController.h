#pragma once
#include "Common/Enums/IOEnums.h"
#include "Core/FileSchemeRegistry/FileSchemeRegistry.h"
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <quuid.h>
#include "Enums/CoreEnums.h"
#include "IO/ReadResult.h"
#include "Physics/Math/MetaDataCalculating.h"
#include "Structures/SessionStructures.h"
#include <expected>
#include <memory>

// Forward declarations в пространствах имен проекта
namespace QSpace::Core {
class DataManager;
class ObjectRegistry;
class DataNode;
class Snapshot;
} // namespace QSpace::Core

namespace QSpace::Core::Controllers {

class ImportFilesWorkflow;
class ImportExperimentWorkflow;
class DataCache;

// ---------------------------------------------------------
// @SECTION: импорт и управление данными
// ---------------------------------------------------------
class ImportController : public QObject {
    Q_OBJECT
  public:
    explicit ImportController(Core::DataManager*        dataManager,
                              Core::ObjectRegistry*     objectRegistry,
                              Core::FileSchemeRegistry* fileSchemeRegistry,
                              QObject*                  parent = nullptr);
    ~ImportController() override;

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

    std::shared_ptr<QSpace::Core::DataNode> getNodeById(const QUuid& nodeId);

    // Временный API до выделения ProjectRestoreWorkflow.
    void prepareNodesForRestoration(const QMap<QString, Session::DataNodeState>& restoringNodes);
    void loadRestoredNodes();

  signals:
    void markSessionDirty(); // Сигнал для ProjectController
    void nodeDataLoaded(const QUuid& nodeId);
    void nodeImported(const QUuid& nodeId);

  public slots:
    void removeNodeObject(const QUuid& id);

    // Совместимый переходный слот. Фактический lazy-load выполняет DataCache.
    void onRequestDataLoad(const QUuid& nodeId);


  private slots:
    void handleFileReady(const QUuid& taskId, IO::ReadResult result, IO::ImportRole role);

  private:
    struct TaskInfo {
        int totalFiles = 1;
    };

    Core::DataManager* m_dataManager;

    Core::ObjectRegistry*     m_objectRegistry;
    Core::FileSchemeRegistry* m_fileSchemeRegistry;

    QMap<QUuid, QUuid> m_taskToExperiment;

    QMap<QUuid, TaskInfo>                 m_activeTasks;
    QMap<QString, Session::DataNodeState> m_restoringNodes;

    bool m_autoGrouping = true;

    std::unique_ptr<ImportFilesWorkflow>      m_importFilesWorkflow;
    std::unique_ptr<ImportExperimentWorkflow> m_importExperimentWorkflow;
    std::unique_ptr<DataCache>                m_dataCache;

    QString                         extractGroupName(const QString& filename);
    std::shared_ptr<Core::Snapshot> findOrCreateSnapshot(const QString& groupName);
    void activateSnapshotInternal(const QUuid& snapshotId, bool isPreview);
};

} // namespace QSpace::Core::Controllers