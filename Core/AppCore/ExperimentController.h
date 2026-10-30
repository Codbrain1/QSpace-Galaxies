#pragma once

#include "Common/Structures/ObjectRegistryStructures.h"
#include <QObject>
#include <QList>
#include <memory>

namespace QSpace::Core {
class ObjectRegistry;
class LayerManager;
class ViewManager;
}

namespace QSpace::Core::Controllers {

class ExperimentController final : public QObject {
    Q_OBJECT
  public:
    explicit ExperimentController(Core::ObjectRegistry* objectRegistry,
                                  Core::LayerManager* layerManager,
                                  Core::ViewManager* viewManager,
                                  QObject* parent = nullptr);

    std::shared_ptr<Core::Experiment> getExperimentById(const QUuid& id) const;
    QList<std::shared_ptr<Core::Experiment>> getExperiments() const;
    bool updatePhysicalUnits(const QUuid& id, const PhysicalUnitsProfile& profile);

  signals:
    void experimentChanged(const QUuid& experimentId);
    void markSessionDirty();

  private:
    Core::ObjectRegistry* m_objectRegistry;
    Core::LayerManager* m_layerManager;
    Core::ViewManager* m_viewManager;
};

} // namespace QSpace::Core::Controllers
