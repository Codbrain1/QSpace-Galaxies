#pragma once
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/ViewManager/ViewManager.h"
#include <QObject>
#include "Session/SessionStorage.h"
#include "Structures/SessionStructures.h"
#include <memory>
#include <optional>

namespace QSpace::Core {
class SessionManager : public QObject { // Исправлена опечатка в имени
    Q_OBJECT
  public:
    explicit SessionManager(QSpace::Core::ObjectRegistry* registry,
                            QSpace::Core::LayerManager*   layerManager,
                            QSpace::Core::ViewManager*    viewManager,
                            QObject*                      parent = nullptr);
    // Сохранение и загрузка проекта
    bool saveProject(const QString& filePath);
    bool loadProject(const QString& filePath);

    // сохранение и загрузка палитр
    bool savePalette(const Visualize::ColorMap& map, const QString& filePath);
    std::optional<Visualize::ColorMap> loadPalette(const QString& filePath);

  signals:
    [[deprecated]]
    void projectLoaded(const QSpace::Session::ProjectState& state);
    void errorOccurred(const QString& msg);

  private:
    QSpace::Session::ProjectDTO   createProjectDTO() const;
    bool                          fromProjectDTO(const QSpace::Session::ProjectDTO& dto);
    QSpace::Core::ObjectRegistry* m_registry;
    QSpace::Core::LayerManager*   m_layerManager;
    QSpace::Core::ViewManager*    m_viewManager;
    std::unique_ptr<QSpace::Session::SessionStorage> m_storage_session;
};
} // namespace QSpace::Core