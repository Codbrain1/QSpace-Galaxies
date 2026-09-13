#include "SessionManager.h"
#include <qfileinfo.h>
#include <qjsonobject.h>
#include <qstringview.h>
#include "Session/ProjectSerializer.h"

namespace QSpace::Core {

SessionManager::SessionManager(QSpace::Core::ObjectRegistry* registry,
                               QSpace::Core::LayerManager*   layerManager,
                               QObject*                      parent)
    : QObject(parent), m_registry(registry), m_layerManager(layerManager) {
    // ВАЖНО: Инициализируем хранилище, иначе будет краш!
    m_storage_session = std::make_unique<QSpace::Session::SessionStorage>();
}

std::optional<QSpace::Session::ProjectState> SessionManager::loadProject(const QString& filePath) {
    auto data = m_storage_session->load(filePath);
    if (data.has_value()) {
        auto projectState = QSpace::Session::ProjectSerializer::deserialize(data.value());
        if (projectState.has_value()) {
            projectState->projectName = QFileInfo(filePath).fileName();
            return projectState;
        }
    }
    return std::nullopt;
}

bool SessionManager::savePalette(const Visualize::ColorMap& map, const QString& filePath) {
    auto jsonFile = QSpace::Session::ProjectSerializer::serializeColorMap(
        map); // TODO преренести реализацию метода в ColorMapSerializer
    QByteArray data = QJsonDocument(jsonFile).toJson(QJsonDocument::Indented);
    return m_storage_session->save(filePath, data);
}

std::optional<Visualize::ColorMap> SessionManager::loadPalette(const QString& filePath) {
    auto        data = m_storage_session->load(filePath);
    QJsonObject jsonObj;
    if (data.has_value()) {
        jsonObj       = QJsonDocument::fromJson(data.value()).object();
        auto colorMap = QSpace::Session::ProjectSerializer::deserializeColorMap(jsonObj);
        return colorMap;
    }
    return std::nullopt;
}

bool SessionManager::saveProject(const QSpace::Session::CurrentSession& curSession) {
    if (curSession.projectFilePath.isEmpty())
        return false;

    QSpace::Session::ProjectState state;
    state.projectName = curSession.projectName;

    for (const auto& node : m_registry->getAllNodes()) {
        QSpace::Session::DataNodeState ds;
        ds.id    = node->id;
        ds.label = node->label;
        ds.path  = node->path;
        // ds.settings = *node->masterSettings.get();
        ds.stats  = node->stats;
        ds.type   = node->type;
        ds.format = node->format;
        // ds.scheme = node->scheme;
        state.nodesStates.append(ds);
    }
    // 2. Сохраняем ВИЗУАЛЬНОЕ ПРЕДСТАВЛЕНИЕ (Слои)
    // Предполагается, что в LayerManager есть метод getAllLayers() возвращающий
    // QList<shared_ptr<Layer>>
    // for (const auto& layer : m_layerManager->getAllLayers()) {
    //     QSpace::Session::LayerState ls;
    //     ls.layerId = layer->layerId;
    //     ls.nodeId  = layer->dataNodeId;

    //     // Предполагаем, что у Views::AbstractView есть метод для получения его ID
    //     if (auto view = layer->view.lock()) {
    //         ls.viewId = view->get();
    //     }

    //     ls.settings = *(layer->settings); // Сохраняем индивидуальные настройки слоя
    //     state.layersStates.append(ls);
    // }

    QByteArray data = QSpace::Session::ProjectSerializer::serialize(state);
    return m_storage_session->save(curSession.projectFilePath, data);
}

} // namespace QSpace::Core