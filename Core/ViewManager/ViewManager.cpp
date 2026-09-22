#include "ViewManager.h"
#include "Visualize/Views/AbstractView.h"
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
    auto id = view->id();

    connect(view.get(), &Visualize::Views::AbstractView::updateRequested, this, [this, id]() {
        emit viewUpdateRequested(id); // Адресное уведомление!
    });

    m_views.emplace(view->id(), std::move(view));

    if (m_mainViewId.isNull()) {
        m_mainViewId = id;
    }

    // 6. Уведомляем систему (AppCore -> MainWindow) о том, что окно создано
    emit viewCreated(id, view->viewType());
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
    if (m_views.contains(id)) {
        m_views.erase(id);
        if (m_mainViewId == id && !m_views.empty()) {
            m_mainViewId = m_views.begin()->first;
        }
        emit viewRemoved(id, m_views[id]->viewType());
    }
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
} // namespace QSpace::Core
