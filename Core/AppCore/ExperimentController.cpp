#include "ExperimentController.h"

#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/ViewManager/ViewManager.h"

namespace QSpace::Core::Controllers {

ExperimentController::ExperimentController(Core::ObjectRegistry* objectRegistry,
                                           Core::LayerManager* layerManager,
                                           Core::ViewManager* viewManager,
                                           QObject* parent)
    : QObject(parent),
      m_objectRegistry(objectRegistry),
      m_layerManager(layerManager),
      m_viewManager(viewManager) {
}

std::shared_ptr<Core::Experiment> ExperimentController::getExperimentById(const QUuid& id) const {
    return m_objectRegistry ? m_objectRegistry->getExperiment(id) : nullptr;
}

QList<std::shared_ptr<Core::Experiment>> ExperimentController::getExperiments() const {
    return m_objectRegistry ? m_objectRegistry->getAllExperiments()
                            : QList<std::shared_ptr<Core::Experiment>>{};
}

bool ExperimentController::updatePhysicalUnits(const QUuid& id,
                                               const PhysicalUnitsProfile& profile) {
    if (!m_objectRegistry || !m_layerManager || !m_viewManager)
        return false;
    if (profile.Km <= 0.0 || profile.Kr <= 0.0 ||
        (profile.haveGamma && profile.gamma <= 1.0))
        return false;

    const auto experiment = m_objectRegistry->getExperiment(id);
    if (!experiment)
        return false;

    experiment->profile = profile;
    for (const auto& snapshot : experiment->snapshots) {
        if (!snapshot)
            continue;
        for (const auto& node : snapshot->components) {
            if (node)
                m_layerManager->applyPhysicalUnits(node->id, profile);
        }
    }

    m_viewManager->setActiveExperiment(profile);
    m_viewManager->renderAllViews();
    emit experimentChanged(id);
    emit markSessionDirty();
    return true;
}

} // namespace QSpace::Core::Controllers
