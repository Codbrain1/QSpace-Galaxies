#pragma once
#include "Visualize/Views/AbstractView.h"
#include <QObject>
#include <qobject.h>
#include <qtmetamacros.h>
#include <quuid.h>
#include <vtkRenderWindow.h>
#include "viewmanager_export.h"
#include <map>
#include <memory>

namespace QSpace::Core {
class VIEWMANAGER_EXPORT ViewManager : public QObject {
    Q_OBJECT
  public:
    explicit ViewManager(QObject* parent = nullptr);
    // Создает окно с заданным ракурсом (удобно для Quad-View)
    QUuid createView(Visualize::Views::ViewType type = Visualize::Views::ViewType::OpenGL3D);
    bool  addView(std::shared_ptr<Visualize::Views::AbstractView> view);

    void                                                   setMainView(const QUuid& viewId);
    QList<std::shared_ptr<Visualize::Views::AbstractView>> getAllViews();

    std::shared_ptr<Visualize::Views::AbstractView> getView(const QUuid& viewId);
    QUuid                                           getMainViewId() const;
    void                                            removeView(const QUuid& id);

    // аргумент принимает указатель на Views::AbstractView
    template <typename Function> void forEachView(Function&& action) {
        for (auto& [id, view] : m_views) {
            action(view);
        }
    }

  public slots:
    void updateAllViews();
    void updateView(const QUuid& id);

    void renderAllViews();
    void renderView(const QUuid& id);


  signals:
    void viewCreated(const QUuid& id, Visualize::Views::ViewType type);
    void viewRemoved(const QUuid& id, Visualize::Views::ViewType type);
    void viewUpdateRequested(const QUuid& id); // В будущем будет использоватся для синхронного
                                               // вращения в нескольких окнах
    void allViewsUpdateRequested();

  private:
    std::map<QUuid, std::shared_ptr<Visualize::Views::AbstractView>> m_views;
    QUuid                                                            m_mainViewId;
};
} // namespace QSpace::Core
