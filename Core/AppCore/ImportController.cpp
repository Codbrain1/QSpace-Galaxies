#include "ImportController.h"
#include "DataCache.h"
#include "ImportExperimentWorkflow.h"
#include "ImportFilesWorkflow.h"



// Подключение внутренних менеджеров ядра
#include "Common/Structures/ObjectRegistryStructures.h"
#include "Core/DataManager/DataManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"


// Системные и утилитарные заголовки
#include "Common/Logger/Logger.h"
#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>
#include <qfileinfo.h>
#include <qloggingcategory.h>
#include <qobjectdefs.h>
#include "IO/ReaderFactory.h"
#include "Structures/FileSchemeStructures.h"
#include <optional>

namespace QSpace::Core::Controllers {

ImportController::ImportController(Core::DataManager*        dataManager,
                                   Core::ObjectRegistry*     objectRegistry,
                                   Core::FileSchemeRegistry* fileSchemeRegistry,
                                   QObject*                  parent)
    : QObject(parent),
      m_dataManager(dataManager),
      m_objectRegistry(objectRegistry),
      m_fileSchemeRegistry(fileSchemeRegistry),
      m_importFilesWorkflow(
          std::make_unique<ImportFilesWorkflow>(dataManager, objectRegistry, fileSchemeRegistry)),
      m_importExperimentWorkflow(
          std::make_unique<ImportExperimentWorkflow>(m_importFilesWorkflow.get(), objectRegistry)),
      m_dataCache(std::make_unique<DataCache>(dataManager, objectRegistry, fileSchemeRegistry)) {
}

ImportController::~ImportController() = default;

void ImportController::initialize() {
    connect(m_dataManager, &Core::DataManager::fileReady, this, &ImportController::handleFileReady);
    connect(m_importFilesWorkflow.get(),
            &ImportFilesWorkflow::taskStarted,
            this,
            [this](const QUuid& taskId, const QUuid& experimentId, int totalFiles) {
                m_activeTasks[taskId].totalFiles = totalFiles;
                if (!experimentId.isNull())
                    m_taskToExperiment.insert(taskId, experimentId);
            });
    connect(m_dataCache.get(), &DataCache::nodeDataLoaded, this, &ImportController::nodeDataLoaded);
    m_dataCache->initialize();

    connect(m_dataManager, &Core::DataManager::ioFinished, this, [this](const QUuid& taskId, bool) {
        m_activeTasks.remove(taskId);
    });
}

void ImportController::importFiles(const QStringList&          paths,
                                   IO::ModelingProgrammVersion version,
                                   const QUuid&                targetExperimentId) {
    QUuid experimentId;
    if (!targetExperimentId.isNull() && m_objectRegistry &&
        m_objectRegistry->getExperiment(targetExperimentId)) {
        experimentId = targetExperimentId;
    }
    m_importFilesWorkflow->start(paths, version, experimentId);
}

void ImportController::importExperiment(const QString&              experimentPath,
                                        IO::ModelingProgrammVersion version) {
    m_importExperimentWorkflow->start(experimentPath, version);
}

void ImportController::importExperiment(const QStringList&          filePaths,
                                        const QString&              experimentName,
                                        IO::ModelingProgrammVersion version) {
    m_importExperimentWorkflow->start(filePaths, experimentName, version);
}

void ImportController::removeNodeObject(const QUuid& id) {
    // Контроллер занимается только структурами данных.
    // ViewController поймает этот сигнал реактивно и зачистит VTK слои в окнах.
    m_objectRegistry->removeObject(id);
    emit markSessionDirty();
}

std::optional<QUuid> ImportController::getExperimentIdByNodePath(const QString& nodePath) const {
    // 1. Получаем все эксперименты из реестра
    auto experiments = m_objectRegistry->getAllExperiments();

    // 2. Проходим по каждому эксперименту и его нодам, сравнивая пути
    for (const auto& experiment : experiments) {
        for (const auto& snapshot : experiment->snapshots) {
            for (const auto& node : snapshot->components) {
                if (node && node->path == nodePath) {
                    return experiment->id; // Возвращаем ID эксперимента, если найдено совпадение
                }
            }
        }
    }

    return std::nullopt; // Если совпадений не найдено, возвращаем nullopt
}

std::optional<QUuid> ImportController::getNodeIdByFilePath(const QString& filePath) const {
    auto nodes = m_objectRegistry->getAllNodes();
    for (const auto& node : nodes) {
        if (node && node->path == filePath) {
            return node->id;
        }
    }
    return std::nullopt;
}

std::shared_ptr<Core::DataNode> ImportController::getNodeById(const QUuid& nodeId) {
    return m_objectRegistry->getNode(nodeId);
}

void ImportController::prepareNodesForRestoration(
    const QMap<QString, Session::DataNodeState>& restoringNodes) {
    m_restoringNodes = restoringNodes;
}

void ImportController::loadRestoredNodes() {
    if (!m_objectRegistry)
        return;

    for (const auto& node : m_objectRegistry->getAllNodes()) {
        if (!node || node->path.isEmpty())
            continue;

        if (!QFileInfo::exists(node->path)) {
            qCWarning(LogCore) << "Restored data file does not exist:" << node->path;
            continue;
        }

        onRequestDataLoad(node->id);
    }
}

void ImportController::handleFileReady(const QUuid&           taskId,
                                       QSpace::IO::ReadResult result,
                                       QSpace::IO::ImportRole role) {
    if (!m_activeTasks.contains(taskId) || role != IO::ImportRole::ProjectData)
        return;

    if (!result.isSuccess()) {
        qCWarning(LogCore) << "Error loading file:" << result.errMessage;
        m_restoringNodes.remove(result.path);
        return;
    }

    // Первичный импорт создаёт новую запись по метаданным.
    // Lazy loading обслуживается исключительно DataCache.

    QString               fileName = QFileInfo(result.path).fileName();
    Visualize::EntityType type     = IO::Utils::getEntityType(fileName);

    // Создаем «каркас» узла (result.data здесь пустой, т.к. прочитан только заголовок)
    auto node = std::make_shared<Core::DataNode>(
        result.data,
        fileName,
        QSpace::Physics::Math::calculateTimestamp(result.timestamp),
        type);

    // Проверяем: мы восстанавливаем сохраненный проект или импортируем новые файлы?
    if (m_restoringNodes.contains(result.path)) {
        auto restoredState = m_restoringNodes.take(result.path);

        node->id    = restoredState.id;
        node->label = restoredState.label;
        // node->masterSettings = restoredState.settings.clone();
        node->path   = restoredState.path;
        node->format = restoredState.format;
        // node->scheme = restoredState.scheme;
        node->type = restoredState.type;
    } else {
        // Обычный новый импорт с диска
        node->path   = result.path;
        node->format = result.format;
        node->label  = fileName;
        node->type   = type;
        // node->scheme           = result.scheme;
        node->stats.pointCount = result.pointCount; // Записываем размер из прочитанного заголовка

        emit markSessionDirty();
    }

    qCInfo(LogCore) << "ImportController::handleFileReady - Скелет ноды создан для:" << fileName
                    << "| Заявлено точек:" << node->stats.pointCount;

    // Флаг: принадлежит ли файл какому-либо упорядоченному эксперименту?
    bool isInsideExperiment = m_taskToExperiment.contains(taskId);

    // 4. Регистрация в ObjectRegistry (Вся логика группировки инкапсулирована там!)
    if (isInsideExperiment) {
        QUuid experimentId = m_taskToExperiment.value(taskId);
        // Реестр сам найдет/создаст нужный Snapshot внутри Эксперимента по timestamp ноды
        m_objectRegistry->registerNodeToExperiment(node, experimentId);
        qCDebug(LogCore) << "Нода" << fileName
                         << "делегирована реестру для интеграции в эксперимент:" << experimentId;
    } else {
        // Одиночный независимый импорт
        m_objectRegistry->registerNode(node);
        qCDebug(LogCore) << "Нода" << fileName << "зарегистрирована на верхнем уровне реестра";
    }

    if (!isInsideExperiment)
        emit nodeImported(node->id);
}

void ImportController::onRequestDataLoad(const QUuid& nodeId) {
    if (m_dataCache)
        m_dataCache->requestLoad(nodeId);
}

QString ImportController::extractGroupName(const QString& filename) {
    // Регулярное выражение для группировки шагов симуляций (например: snap_001, step-500)
    static QRegularExpression regex(R"((snap(shot)?|step)[_\-]?\d+)",
                                    QRegularExpression::CaseInsensitiveOption);
    auto                      match = regex.match(filename);
    return match.hasMatch() ? match.captured(0) : QString();
}

std::shared_ptr<Core::Snapshot> ImportController::findOrCreateSnapshot(const QString& groupName) {
    auto existing = m_objectRegistry->findSnapshotByName(groupName);
    if (existing)
        return existing;

    auto newContainer = std::make_shared<Core::Snapshot>(groupName);
    m_objectRegistry->registerSnapshot(newContainer);
    return newContainer;
}

} // namespace QSpace::Core::Controllers
