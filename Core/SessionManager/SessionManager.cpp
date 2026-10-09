#include "SessionManager.h"
#include "Common/Structures/SessionStructures.h"
#include "Core/LayerManager/LayerManagerSerializer.h"
#include "Core/ObjectRegistry/ObjectRegistrySerializer.h"
#include "Core/ViewManager/ViewManager.h"
#include "Core/ViewManager/ViewManagerSerializer.h"
#include "Visualize/ColorMapManager/ColorMapManagerSerializer.h"
#include <qfileinfo.h>
#include <qjsonobject.h>
#include <qstringview.h>
#include "Session/ProjectSerializer.h"
#include "Session/Reflection.h"

namespace QSpace::Core {

SessionManager::SessionManager(QSpace::Core::ObjectRegistry* registry,
                               QSpace::Core::LayerManager*   layerManager,
                               QSpace::Core::ViewManager*    viewManager,
                               QObject*                      parent)
    : QObject(parent),
      m_registry(registry),
      m_layerManager(layerManager),
      m_viewManager(viewManager) {
    // ВАЖНО: Инициализируем хранилище, иначе будет краш!
    m_storage_session = std::make_unique<QSpace::Session::SessionStorage>();
}

// ===================================================
// savePallete() -- сохраняет палитру
// loadPallete() -- загружает палитру
// ===================================================
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

// ===================================================
// saveProject() -- создает DTO и сохраняет проект
// loadProject() -- загружает DTO и восстанавливает состояние приложения
// ===================================================
bool SessionManager::saveProject(const QString& filePath) {
    const QSpace::Session::ProjectDTO dto            = createProjectDTO();
    const auto                        projVariantMap = QSpace::Reflection::gadgetToVariantMap(dto);
    return m_storage_session->save(filePath, data);
}

bool SessionManager::loadProject(const QString& filePath) {
    const auto data = m_storage_session->load(filePath);
    if (!data.has_value())
        return false;
    auto dto = Session::ProjectSerializer::deserialize(*data);
    if (!dto.has_value())
        return false;

    return fromProjectDTO(dto.value());
}

QSpace::Session::ProjectDTO SessionManager::createProjectDTO() const {
    QSpace::Session::ProjectDTO dto;

    dto.objregDTO      = ObjectRegistrySerializer::toDTO(m_registry).value();
    dto.layermanDTO    = LayerManagerSerializer::toDTO(m_layerManager).value();
    dto.viewmanDTO     = ViewManagerSerializer::toDTO(m_viewManager).value();
    dto.colormapmanDTO = QSpace::Visualize::ColorMapManagerSerializer::toDTO();

    return dto;
}

bool SessionManager::fromProjectDTO(const QSpace::Session::ProjectDTO& dto) {
    bool is_succses = ObjectRegistrySerializer::fromDTO(dto.objregDTO, m_registry);
    is_succses      = is_succses && ViewManagerSerializer::fromDTO(dto.viewmanDTO, m_viewManager);
    QSpace::Visualize::ColorMapManagerSerializer::fromDTO(dto.colormapmanDTO);

    LayerManagerSerializer::NodeResolver nodeRes =
        [&](const QUuid& id) -> std::shared_ptr<DataNode> { return m_registry->getNode(id); };

    LayerManagerSerializer::ViewResolver viewRes =
        [&](const QUuid& id) -> std::shared_ptr<QSpace::Visualize::Views::AbstractView> {
        return m_viewManager->getView(id);
    };

    is_succses = is_succses &&
                 LayerManagerSerializer::fromDTO(dto.layermanDTO, m_layerManager, nodeRes, viewRes);
    return is_succses;
}
} // namespace QSpace::Core