#include "LayerManager.h"
#include "Common/Logger/Logger.h"
#include "Core/StyleManager/StyleManager.h"
#include "Visualize/Layers/LayerFactory.h"
#include "Visualize/Views/View3D/AbstractView3D.h"
#include <qcontainerfwd.h>
#include "Enums/LayerEnums.h"
#include "Session/Reflection.h"
#include <memory>

namespace QSpace::Core {

LayerManager::LayerManager(QObject* parent) : QObject(parent) {
}

LayerManager::~LayerManager() = default;

QUuid LayerManager::createLayer(std::shared_ptr<DataNode> node,
                                std::shared_ptr<Visualize::Views::AbstractView> view) {
    return createLayer(std::move(node), std::move(view),
                       Visualize::Layers::RenderLayerType::SPH);
}

QUuid LayerManager::createLayer(std::shared_ptr<DataNode> node,
                                std::shared_ptr<Visualize::Views::AbstractView> view,
                                Visualize::Layers::RenderLayerType renderType) {
    if (!node || !view || !node->isLoaded()) {
        qCWarning(LogCore) << "LayerManager::createLayer - Invalid node or view";
        return QUuid();
    }

    auto layer = std::make_shared<Visualize::Layers::Layer>(node, view);
    auto renderEngine = Visualize::Layers::LayerFactory::createLayerRenderer(node, renderType);
    if (!renderEngine)
        return QUuid();

    // 3. Инициализируем движок данными и настройками
    layer->assignEngine(renderEngine);
    layer->update();
    layer->getSettings()->setColorByField("Mass");

    // 5. Регистрация в индексах
    m_layers.insert(layer->layerId(), layer);
    m_nodeToLayers[node->id].append(layer->layerId());
    m_viewToLayers[view.get()].append(layer->layerId());

    emit layerCreated(layer->layerId());
    return layer->layerId();
}

std::shared_ptr<Visualize::Layers::Layer> LayerManager::createLayerCopy(const QUuid& etalonLayerId) {
    const auto etalonIt = m_layers.constFind(etalonLayerId);
    if (etalonIt == m_layers.cend() || !etalonIt.value()) {
        qCWarning(LogCore) << "LayerManager::createLayerCopy: source layer not found"
                           << etalonLayerId;
        return nullptr;
    }

    const auto& etalonLayer = etalonIt.value();
    const auto  view        = etalonLayer->getView().lock();
    if (!view) {
        qCWarning(LogCore) << "LayerManager::createLayerCopy: source layer has no live view"
                           << etalonLayerId;
        return nullptr;
    }

    auto layer = std::make_shared<Visualize::Layers::Layer>(etalonLayer->name(), view);
    if (!layer) {
        qCWarning(LogCore) << "LayerManager::createLayerCopy: failed to allocate layer";
        return nullptr;
    }

    auto renderEngine = Visualize::Layers::LayerFactory::createLayerRenderer(
        Visualize::Layers::RenderLayerType::SPH);
    if (!renderEngine) {
        qCWarning(LogCore) << "LayerManager::createLayerCopy: failed to create render engine"
                           << etalonLayerId;
        return nullptr;
    }

    layer->assignEngine(renderEngine);
    auto layerSettings = layer->getSettings();
    if (!layerSettings) {
        qCWarning(LogCore) << "LayerManager::createLayerCopy: render engine has no settings"
                           << etalonLayerId;
        return nullptr;
    }

    if (const auto etalonSettings = etalonLayer->getSettings()) {
        const auto settingsMap = QSpace::Reflection::QObjectToVariantMap(etalonSettings.get());
        if (!settingsMap.isEmpty()) {
            const auto result =
                QSpace::Reflection::variantMapToQObject(settingsMap, layerSettings.get());
            if (!result.success) {
                qCWarning(LogCore) << "LayerManager::createLayerCopy: failed to restore settings"
                                   << etalonLayerId;
                return nullptr;
            }
        } else {
            layerSettings->setColorByField(QStringLiteral("Mass"));
        }
    } else {
        layerSettings->setColorByField(QStringLiteral("Mass"));
    }

    layer->update();
    if (!registerLayer(layer)) {
        qCWarning(LogCore) << "LayerManager::createLayerCopy: failed to register copy"
                           << etalonLayerId;
        return nullptr;
    }

    return layer;
}

bool LayerManager::registerLayer(std::shared_ptr<Visualize::Layers::Layer> layer) {
    if (!layer) {
        return false;
    }

    const QUuid id = layer->layerId();
    if (m_layers.contains(id)) {
        qCWarning(LogCore) << "Layer with ID already exists:" << id;
        return false;
    }

    m_layers.insert(id, layer);

    if (!layer->dataNodeId().isNull()) {
        m_nodeToLayers[layer->dataNodeId()].append(id);
    }

    if (auto view = layer->getView().lock()) {
        m_viewToLayers[view.get()].append(id);
    }

    emit layerCreated(id);
    return true;
}

void LayerManager::removeLayer(const QUuid& layerId) {
    if (!m_layers.contains(layerId))
        return;

    auto layer = m_layers[layerId];
    if (!layer) {
        return;
    }

    auto rawViewPtr = layer->getView().lock().get(); // Получаем адрес окна, если оно живо

    // Отвязываем от View (только если окно еще существует)
    if (rawViewPtr) {
        // Удаляем из индекса окна
        if (m_viewToLayers.contains(rawViewPtr)) {
            m_viewToLayers[rawViewPtr].removeOne(layerId);
            if (m_viewToLayers[rawViewPtr].isEmpty())
                m_viewToLayers.remove(rawViewPtr);
        }
        if (auto view3D = dynamic_cast<QSpace::Visualize::Views::AbstractView3D*>(rawViewPtr)) {
            view3D->detachRenderLayer(layerId);
        }
    }

    // 2. Ищем ноду безопасным способом без неявного создания элемента
    auto it = m_nodeToLayers.find(layer->dataNodeId());

    if (it != m_nodeToLayers.end()) {
        // Если нода найдена, удаляем слой из её списка
        it.value().removeOne(layerId);

        // Хитрый ход: если после удаления слоя список для этой ноды опустел,
        // удаляем и саму ноду из мапы, чтобы не засорять память
        if (it.value().isEmpty()) {
            m_nodeToLayers.erase(it);
        }
    }
    m_layers.remove(layerId);

    emit layerRemoved(layerId);
}

void LayerManager::removeAllLayersForNode(const QUuid& nodeId) {
    // Используем временный список, так как removeLayer модифицирует m_nodeToLayers
    QList<QUuid> toRemove = m_nodeToLayers.value(nodeId);
    for (const auto& id : toRemove) {
        removeLayer(id);
    }
}

void LayerManager::clear() {
    const auto layerIds = m_layers.keys();
    for (const auto& layerId : layerIds)
        removeLayer(layerId);

    m_layers.clear();
    m_nodeToLayers.clear();
    m_viewToLayers.clear();
}

void LayerManager::removeAllLayersForView(Visualize::Views::AbstractView* view) {
    if (!m_viewToLayers.contains(view))
        return;

    QList<QUuid> toRemove = m_viewToLayers.value(view);
    for (const auto& id : toRemove) {
        removeLayer(id);
    }
}

void LayerManager::populateNewView(std::shared_ptr<Visualize::Views::AbstractView> newView,
                                   const QList<std::shared_ptr<DataNode>>&         allNodes) {
    for (const auto& node : allNodes) {
        createLayer(node, newView);
    }
}

std::shared_ptr<Visualize::Layers::Layer> LayerManager::getLayer(const QUuid& layerId) const {
    return m_layers.value(layerId, nullptr);
}

void LayerManager::applyPhysicalUnits(const QUuid& nodeId, const PhysicalUnitsProfile& profile) {
    for (const auto& layer : getLayersForNode(nodeId)) {
        if (const auto settings = layer ? layer->getSettings() : nullptr)
            settings->setPhysicalUnitsParameters(profile.Km, profile.Kr, profile.gamma, profile.haveGamma);
    }
}

QList<std::shared_ptr<Visualize::Layers::Layer>> LayerManager::getLayersForNode(const QUuid& nodeId) const {
    QList<std::shared_ptr<Visualize::Layers::Layer>> result;
    for (const auto& id : m_nodeToLayers.value(nodeId)) {
        if (m_layers.contains(id))
            result.append(m_layers[id]);
    }
    return result;
}

void LayerManager::updateNodeMasterSettings(const QUuid& nodeId) {
    const auto layers = getLayersForNode(nodeId);
    const auto node   = (!layers.isEmpty() && layers.front()) ? layers.front()->dataNode() : nullptr;

    for (const auto& layer : layers) {
        if (!layer || !layer->IsSyncedWithMaster())
            continue;

        if (node)
            layer->setData(node);

        layer->update();
    }
}

// 1. Создание слоев — оставляем как есть, это надежно
void LayerManager::createLayersForContainer(std::shared_ptr<Snapshot>                       container,
                                            std::shared_ptr<Visualize::Views::AbstractView> view) {
    if (!container || !view)
        return;

    for (auto& node : container->components) {
        this->createLayer(node, view);
    }
}

// 2. Управление видимостью — добавляем флаг блокировки рендера
void LayerManager::setSnapshotVisibility(std::shared_ptr<Snapshot> container, bool visible) {
    if (!container)
        return;

    // Обновляем узлы текущего контейнера
    for (auto& node : container->components) {
        // Мы используем прямой доступ к ID, это быстро
        auto layerIds = m_nodeToLayers.value(node->id);
        for (const auto& id : layerIds) {
            if (auto layer = m_layers.value(id)) {
                if (layer)
                    layer->setVisible(visible);
            }
        }
    }
}

void LayerManager::setNodeVisibility(const QUuid& nodeId, bool visible) {
    auto layers = getLayersForNode(nodeId);
    for (auto& layer : layers) {
        if (layer->IsSyncedWithMaster()) {
            layer->setVisible(visible);
        }
    }
}

bool LayerManager::isNodeVisible(const QUuid& nodeId) {
    auto layers = getLayersForNode(nodeId);
    for (auto& layer : layers) {
        if (layer->getSettings()->isVisible()) {
            return true;
        }
    }
    return false;
}
} // namespace QSpace::Core