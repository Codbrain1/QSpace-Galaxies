#include "LayerManager.h"
#include "Common/Logger/Logger.h"
#include "Core/StyleManager/StyleManager.h"
#include "Visualize/Layers/LayerFactory.h"
#include "Visualize/Views/View3D/AbstractView3D.h"
#include "Enums/LayerEnums.h"
#include <memory>

namespace QSpace::Core {

LayerManager::LayerManager(QObject* parent) : QObject(parent) {
}

LayerManager::~LayerManager() = default;

QUuid LayerManager::createLayer(std::shared_ptr<DataNode> node, std::shared_ptr<Visualize::Views::AbstractView> view) {
    if (!node || !view || !node->isLoaded()) {
        qCWarning(LogCore) << "LayerManager::createLayer - Invalid node or view";
        return QUuid();
    }

    // 1. Создаем обертку
    auto layer = std::make_shared<Visualize::Layers::Layer>(node, view);

    // 2. Создаем движок через фабрику
    auto renderEngine = Visualize::Layers::LayerFactory::createLayerRenderer(
        node,
        Visualize::Layers::RenderLayerType::SPH); // CRITICAL добавить возможность создания
                                                  // разных типов слоев
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
    auto it = m_layers.find(etalonLayerId);
    if (it == m_layers.end())
        return nullptr;
    auto etalonLayer = it.value();
    auto view        = etalonLayer->getView().lock();

    auto layer = std::make_shared<Visualize::Layers::Layer>(etalonLayer->name(), view);

    auto renderEngine = Visualize::Layers::LayerFactory::createLayerRenderer(Visualize::Layers::RenderLayerType::SPH);

    if (!renderEngine)
        return nullptr;

    layer->assignEngine(renderEngine);
    layer->update();
    // TODO: заменить на методы рефлексии
    // layer->getSettings()->fromVariantMap(etalonLayer->getSettings()->toVariantMap());

    // 5. Регистрация в индексах
    m_layers.insert(layer->layerId(), layer);
    m_viewToLayers[view.get()].append(layer->layerId());
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

QList<std::shared_ptr<Visualize::Layers::Layer>> LayerManager::getLayersForNode(const QUuid& nodeId) const {
    QList<std::shared_ptr<Visualize::Layers::Layer>> result;
    for (const auto& id : m_nodeToLayers.value(nodeId)) {
        if (m_layers.contains(id))
            result.append(m_layers[id]);
    }
    return result;
}

void LayerManager::updateNodeMasterSettings(const QUuid& nodeId) {
    // auto layers = getLayersForNode(nodeId);
    // for (auto& layer : layers) {
    //     if (layer->IsSyncedWithMaster()) {
    //         layer->update();
    //     }
    // }
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