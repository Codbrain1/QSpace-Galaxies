#include "ViewManager.h"
#include "Visualize/Views/AbstractView.h"
#include "Visualize/Views/View3D/AbstractView3D.h"
#include "Physics/DimensionConverter/PhysicalUnits.h"
#include "Visualize/Views/ViewFactory.h"
#include <qobject.h>
#include <quuid.h>
#include <memory>

namespace QSpace::Core {
ViewManager::ViewManager(QObject* parent) : QObject(parent) {
}

QUuid ViewManager::createView(Visualize::Views::ViewType type) {
    auto newView = QSpace::Visualize::Views::ViewFactory::createView(type);
    // Проверка на случай неудачного создания или нереализованного типа (как Widget_2D сейчас)
    if (!newView) {
        return QUuid();
    }
    auto id = newView->id();

    connect(newView.get(), &Visualize::Views::AbstractView::updateRequested, this, [this, id]() {
        emit viewUpdateRequested(id); // Адресное уведомление!
    });


    // 4. Перемещаем владение объектом в контейнер менеджера
    applyActiveExperimentToView(newView);
    m_views.emplace(newView->id(), std::move(newView));

    // 5. Устанавливаем главное окно, если оно еще не задано
    if (m_mainViewId.isNull()) {
        m_mainViewId = id;
    }

    // 6. Уведомляем систему (AppCore -> MainWindow) о том, что окно создано
    emit viewCreated(id, type);

    return id;
}

bool ViewManager::addView(std::shared_ptr<Visualize::Views::AbstractView> view) {
    if (!view) {
        return false;
    }
    const auto id   = view->id();
    const auto type = view->viewType();
    if (id.isNull() || m_views.contains(id)) {
        return false;
    }

    connect(view.get(), &Visualize::Views::AbstractView::updateRequested, this, [this, id]() {
        emit viewUpdateRequested(id); // Адресное уведомление!
    });

    applyActiveExperimentToView(view);
    m_views.emplace(view->id(), std::move(view));

    if (m_mainViewId.isNull()) {
        m_mainViewId = id;
    }

    // 6. Уведомляем систему (AppCore -> MainWindow) о том, что окно создано
    emit viewCreated(id, type);
    return true;
}

QList<std::shared_ptr<Visualize::Views::AbstractView>> ViewManager::getAllViews() {
    QList<std::shared_ptr<Visualize::Views::AbstractView>> views;
    for (const auto& [id, view] : m_views) {
        views.append(view);
    }
    return views;
}

std::shared_ptr<Visualize::Views::AbstractView> ViewManager::getView(const QUuid& viewId) {
    if (m_views.contains(viewId)) {
        return m_views[viewId];
    }
    return nullptr;
}

void ViewManager::setMainView(const QUuid& viewId) {
    if (m_views.contains(viewId)) {
        m_mainViewId = viewId;
    }
}

QUuid ViewManager::getMainViewId() const {
    return m_mainViewId;
}

void ViewManager::removeView(const QUuid& id) {
    const auto it = m_views.find(id);
    if (it == m_views.end())
        return;

    const auto type = it->second ? it->second->viewType()
                                 : Visualize::Views::ViewType::OpenGL3D;
    m_views.erase(it);

    if (m_mainViewId == id)
        m_mainViewId = m_views.empty() ? QUuid() : m_views.begin()->first;

    emit viewRemoved(id, type);
}

void ViewManager::clear() {
    const auto ids = [&]() {
        QList<QUuid> result;
        for (const auto& [id, view] : m_views)
            result.append(id);
        return result;
    }();

    for (const auto& id : ids)
        removeView(id);

    m_mainViewId = QUuid();
    m_hasActiveExperiment = false;
}

void ViewManager::updateAllViews() {
    for (auto& [id, view] : m_views) {
        view->render();
    }
}

void ViewManager::updateView(const QUuid& id) {
    if (m_views.contains(id)) {
        m_views[id]->render();
    }
}

void ViewManager::renderAllViews() {
    for (auto& [id, view] : m_views) {
        view->render();
    }
}

void ViewManager::renderView(const QUuid& id) {
    if (m_views.contains(id)) {
        m_views[id]->render();
    }
}

void ViewManager::applyActiveExperimentToView(
    const std::shared_ptr<Visualize::Views::AbstractView>& view) {
    if (!m_hasActiveExperiment || !view)
        return;
    if (auto view3d = std::dynamic_pointer_cast<Visualize::Views::AbstractView3D>(view)) {
        const auto units = Physics::PhysicalUnits::fromSimParams(
            m_activeExperimentProfile.Km,
            m_activeExperimentProfile.Kr,
            m_activeExperimentProfile.gamma,
            m_activeExperimentProfile.haveGamma);
        if (auto settings = view3d->sceneSettings())
            settings->axis()->setLengthScale(units.lengthScale);
    }
}

void ViewManager::setActiveExperiment(const PhysicalUnitsProfile& profile) {
    m_activeExperimentProfile = profile;
    m_hasActiveExperiment = true;
    for (auto& [id, view] : m_views)
        applyActiveExperimentToView(view);
}
} // namespace QSpace::Core
