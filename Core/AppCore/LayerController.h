#pragma once

#include "Visualize/Layers/Layer.h"
#include <QObject>
#include <QUuid>
#include <memory>

namespace QSpace::Core {
class ObjectRegistry;
class LayerManager;
class ViewManager;
class DataNode;
}

namespace QSpace::Core::Controllers {

class LayerController final : public QObject {
    Q_OBJECT
  public:
    explicit LayerController(Core::ObjectRegistry* objectRegistry,
                             Core::LayerManager*   layerManager,
                             Core::ViewManager*    viewManager,
                             QObject*              parent = nullptr);

    std::shared_ptr<Visualize::Layers::Layer> getLayerById(const QUuid& layerId) const;

  public slots:
    void createDefaultLayers(const QUuid& nodeId);
    void createLayer(const QUuid&                          nodeId,
                     const QUuid&                          viewId,
                     Visualize::Layers::RenderLayerType renderType);
    void duplicateLayer(const QUuid& layerId);
    void removeLayer(const QUuid& layerId);
    void setVisible(const QUuid& layerId, bool visible);
    void onNodeSelectionActivated(const QUuid& nodeId);
    void onNodeDataLoaded(const QUuid& nodeId);

  signals:
    void layerCreated(const QUuid& layerId);
    void layerChanged(const QUuid& layerId);
    void layerRemoved(const QUuid& layerId);
    void sceneUpdateRequested();
    void markSessionDirty();

  private:
    Core::ObjectRegistry* m_objectRegistry;
    Core::LayerManager*   m_layerManager;
    Core::ViewManager*    m_viewManager;
};

} // namespace QSpace::Core::Controllers
