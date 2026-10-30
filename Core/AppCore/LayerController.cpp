#include "LayerController.h"

#include "Common/Structures/ObjectRegistryStructures.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/ViewManager/ViewManager.h"

namespace QSpace::Core::Controllers {

LayerController::LayerController(Core::ObjectRegistry* objectRegistry,
                                 Core::LayerManager*   layerManager,
                                 Core::ViewManager*    viewManager,
                                 QObject*              parent)
    : QObject(parent),
      m_objectRegistry(objectRegistry),
      m_layerManager(layerManager),
      m_viewManager(viewManager) {
}

std::shared_ptr<Visualize::Layers::Layer> LayerController::getLayerById(const QUuid& layerId) const {
    return m_layerManager ? m_layerManager->getLayer(layerId) : nullptr;
}

void LayerController::createDefaultLayers(const QUuid& nodeId) {
    if (!m_objectRegistry || !m_layerManager || !m_viewManager)
        return;

    const auto node = m_objectRegistry->getNode(nodeId);
    if (!node)
        return;

    m_viewManager->forEachView([this, &node](std::shared_ptr<Visualize::Views::AbstractView> view) {
        const auto layerId = m_layerManager->createLayer(node, view);
        if (const auto layer = m_layerManager->getLayer(layerId)) {
            if (const auto experiment = m_objectRegistry->getExperimentForNode(node->id))
                m_layerManager->applyPhysicalUnits(node->id, experiment->profile);
            emit layerCreated(layerId);
        }
    });
    emit sceneUpdateRequested();
    emit markSessionDirty();
}

void LayerController::createLayer(const QUuid&                          nodeId,
                                  const QUuid&                          viewId,
                                  Visualize::Layers::RenderLayerType renderType) {
    if (!m_objectRegistry || !m_layerManager || !m_viewManager)
        return;

    const auto node = m_objectRegistry->getNode(nodeId);
    const auto view = m_viewManager->getView(viewId);
    if (!node || !view)
        return;

    const auto layerId = m_layerManager->createLayer(node, view, renderType);
    if (layerId.isNull())
        return;

    if (const auto experiment = m_objectRegistry->getExperimentForNode(node->id))
        m_layerManager->applyPhysicalUnits(node->id, experiment->profile);
    emit layerCreated(layerId);
    emit sceneUpdateRequested();
    emit markSessionDirty();
}

void LayerController::duplicateLayer(const QUuid& layerId) {
    if (!m_layerManager)
        return;

    const auto layer = m_layerManager->createLayerCopy(layerId);
    if (!layer)
        return;

    emit layerCreated(layer->layerId());
    emit sceneUpdateRequested();
    emit markSessionDirty();
}

void LayerController::removeLayer(const QUuid& layerId) {
    if (!m_layerManager || !m_layerManager->getLayer(layerId))
        return;

    m_layerManager->removeLayer(layerId);
    emit layerRemoved(layerId);
    emit sceneUpdateRequested();
    emit markSessionDirty();
}

void LayerController::setVisible(const QUuid& layerId, bool visible) {
    const auto layer = getLayerById(layerId);
    if (!layer)
        return;

    layer->setVisible(visible);
    emit layerChanged(layerId);
    emit sceneUpdateRequested();
    emit markSessionDirty();
}

void LayerController::onNodeSelectionActivated(const QUuid& nodeId) {
    if (!m_objectRegistry || !m_layerManager || !m_objectRegistry->getNode(nodeId))
        return;

    if (m_objectRegistry->getOrLoadNodeData(nodeId)) {
        if (const auto experiment = m_objectRegistry->getExperimentForNode(nodeId))
            m_viewManager->setActiveExperiment(experiment->profile);
        m_layerManager->updateNodeMasterSettings(nodeId);
        emit sceneUpdateRequested();
    }
}

void LayerController::onNodeDataLoaded(const QUuid& nodeId) {
    if (!m_layerManager)
        return;

    m_layerManager->updateNodeMasterSettings(nodeId);
    emit sceneUpdateRequested();
}

} // namespace QSpace::Core::Controllers
