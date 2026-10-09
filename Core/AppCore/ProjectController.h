#pragma once
#include "Common/Enums/VisualizeBaseEnums.h"
#include <QObject>
#include <QString>
#include "Structures/SessionStructures.h"

namespace QSpace::Core {
class SessionManager;
class ObjectRegistry;
class DataManager;
} // namespace QSpace::Core

namespace QSpace::Core::Controllers {
class DataController;
}

namespace QSpace::Core::Controllers {
// ---------------------------------------------------------
// @SECTION: обработка проектов и палитр
// ---------------------------------------------------------
class ProjectController : public QObject {
    Q_OBJECT
  public:
    explicit ProjectController(Core::SessionManager* sessionManager,
                               Core::ObjectRegistry* objectRegistry,
                               Core::DataManager*    dataManager,
                               DataController*       dataController,
                               QObject*              parent = nullptr);

    void initialize();

    void createNewProject(const QString& projectName);
    void saveCurrentProject();
    void saveCurrentProjectAs(const QString& projectPath);
    void openProject(const QString& projectPath);

    void savePalette(const Visualize::ColorMap& map, const QString& filePath);
    void loadPalette(const QString& filePath);

  signals:
    /**
     * @brief сигнал о запросе пути для сохранения проекта
     */
    void requestSavePathFromUI();
    /**
     * @brief уведомляет об изменении состояния сессии
     */
    void sessionStateChanged(QSpace::Session::CurrentSession session);
    void paletteLoaded(QSpace::Visualize::ColorMap colorMap);

  private:
    Core::SessionManager* m_sessionManager;
    Core::ObjectRegistry* m_objectRegistry;
    Core::DataManager*    m_dataManager;
    DataController*       m_dataController; // Нужен для передачи состояния при загрузке

    [[deprecated]]
    Session::CurrentSession m_sessionState;
};

} // namespace QSpace::Core::Controllers