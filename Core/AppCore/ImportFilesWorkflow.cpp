#include "ImportFilesWorkflow.h"
#include "Core/DataManager/DataManager.h"
#include "Core/FileSchemeRegistry/FileSchemeRegistry.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "IO/ReaderFactory.h"
#include <QFileInfo>

namespace QSpace::Core::Controllers {

ImportFilesWorkflow::ImportFilesWorkflow(Core::DataManager* dataManager,
                                         Core::ObjectRegistry* objectRegistry,
                                         Core::FileSchemeRegistry* fileSchemeRegistry,
                                         QObject* parent)
    : QObject(parent),
      m_dataManager(dataManager),
      m_objectRegistry(objectRegistry),
      m_fileSchemeRegistry(fileSchemeRegistry) {
}

void ImportFilesWorkflow::start(const QStringList& paths,
                                IO::ModelingProgrammVersion version,
                                const QUuid& targetExperimentId) {
    if (paths.isEmpty() || !m_dataManager || !m_fileSchemeRegistry)
        return;

    QList<IO::BatchTask> tasks;
    for (const auto& path : paths) {
        const auto fileInfo = QFileInfo(path);
        IO::PresetKey key;
        key.format = IO::Utils::getFormat(fileInfo.fileName());
        key.type = IO::Utils::getEntityType(fileInfo.fileName());
        key.mpv = version;
        const auto schemeId = m_fileSchemeRegistry->findPresetId(key);
        if (!schemeId.has_value())
            continue;
        const auto scheme = m_fileSchemeRegistry->getReadScheme(*schemeId);
        if (scheme.has_value())
            tasks.append({path, *scheme});
    }

    if (tasks.isEmpty())
        return;

    const auto taskId = tasks.size() == 1
        ? m_dataManager->importDataAsync(tasks.first().path, tasks.first().scheme)
        : m_dataManager->importBatchDataAsync(tasks);
    if (!taskId.isNull())
        emit taskStarted(taskId, targetExperimentId, tasks.size());
}

} // namespace QSpace::Core::Controllers
