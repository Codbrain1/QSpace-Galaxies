#include "DataCache.h"
#include "Common/Logger/Logger.h"
#include "Core/DataManager/DataManager.h"
#include "Core/FileSchemeRegistry/FileSchemeRegistry.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Common/Structures/ObjectRegistryStructures.h"
#include <QFileInfo>

namespace QSpace::Core::Controllers {

DataCache::DataCache(Core::DataManager*        dataManager,
                     Core::ObjectRegistry*     objectRegistry,
                     Core::FileSchemeRegistry* fileSchemeRegistry,
                     QObject*                  parent)
    : QObject(parent),
      m_dataManager(dataManager),
      m_objectRegistry(objectRegistry),
      m_fileSchemeRegistry(fileSchemeRegistry) {
}

void DataCache::initialize() {
    connect(m_dataManager, &Core::DataManager::fileReady, this, &DataCache::handleFileReady);
    connect(m_dataManager, &Core::DataManager::ioFinished, this, &DataCache::handleIoFinished);
    connect(m_objectRegistry, &Core::ObjectRegistry::dataLoadRequested, this, &DataCache::requestLoad);
}

void DataCache::setCapacity(int capacity) {
    if (!m_objectRegistry || capacity < 1)
        return;
    m_objectRegistry->setCacheCapacity(capacity);
}

int DataCache::capacity() const {
    return m_objectRegistry ? m_objectRegistry->cacheCapacity() : 0;
}

void DataCache::requestLoad(const QUuid& nodeId) {
    if (m_loadingNodes.contains(nodeId) || !m_dataManager || !m_objectRegistry)
        return;

    const auto node = m_objectRegistry->getNode(nodeId);
    if (!node || node->path.isEmpty())
        return;

    std::optional<IO::ReadScheme> scheme;
    if (!node->scheme.isNull() && m_fileSchemeRegistry)
        scheme = m_fileSchemeRegistry->getReadScheme(node->scheme);

    if (!scheme.has_value() && m_fileSchemeRegistry) {
        IO::PresetKey key;
        key.format = node->format;
        key.type = node->type;
        key.mpv = IO::ModelingProgrammVersion::V2;
        const auto schemeId = m_fileSchemeRegistry->findPresetId(key);
        if (schemeId.has_value()) {
            node->scheme = schemeId.value();
            scheme = m_fileSchemeRegistry->getReadScheme(schemeId.value());
        }
    }

    if (!scheme.has_value()) {
        qCWarning(LogCore) << "No read scheme found for node:" << node->path;
        return;
    }

    const auto taskId = m_dataManager->importDataAsync(node->path, *scheme, IO::ImportRole::ProjectData);
    if (taskId.isNull())
        return;

    m_loadingNodes.insert(nodeId);
    m_tasks.insert(taskId, LoadingTask{nodeId});
}

void DataCache::handleFileReady(const QUuid& taskId, const IO::ReadResult& result, IO::ImportRole role) {
    if (role != IO::ImportRole::ProjectData || !m_tasks.contains(taskId) || !result.isSuccess())
        return;

    const auto nodeId = m_tasks.value(taskId).nodeId;
    m_loadingNodes.remove(nodeId);
    m_objectRegistry->updateNodeData(nodeId, result.data, result.timestamp);

    emit nodeDataLoaded(nodeId);
}

void DataCache::handleIoFinished(const QUuid& taskId, bool) {
    const auto task = m_tasks.take(taskId);
    if (!task.nodeId.isNull())
        m_loadingNodes.remove(task.nodeId);
}

} // namespace QSpace::Core::Controllers
