#pragma once
#include <QObject>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <quuid.h>
#include <vtkRenderWindow.h>
#include "DataController.h"
#include "Models/DataTreeModel/DataTreeModel.h"
#include "ProjectController.h"
#include "Structures/SessionStructures.h"
#include "VideoController.h"
#include "ViewController.h"
#include <memory>

// Forward declarations для ускорения компиляции
namespace QSpace::Core {
class TaskManager;
class DataManager;
class ObjectRegistry;
class ViewManager;
class LayerManager;
class SessionManager;
class FileSchemeRegistry;
// class VideoExportManager;
} // namespace QSpace::Core

namespace QSpace::Models {
class DataTreeModel;
} // namespace QSpace::Models

// Forward declarations Контроллеров
namespace QSpace::Core::Controllers {
class ViewController;
class DataController;
class ProjectController;
class VideoController;
} // namespace QSpace::Core::Controllers

namespace QSpace::Core {
class AppCore : public QObject {
    Q_OBJECT
  public:
    explicit AppCore(QObject* parent = nullptr);
    ~AppCore();
    // ---------------------------------------------------------
    // @SECTION: инициализация
    // ---------------------------------------------------------
    /**
     * @brief initialize --- соединяет сигналы и слоты контроллеров из AppCore друг с другом
     */
    void initialize();

    // ---------------------------------------------------------
    // @SECTION: модели данных
    // ---------------------------------------------------------
    Models::DataTreeModel* dataTreeModel() const;

    // ---------------------------------------------------------
    // @SECTION: контроллеры
    // ---------------------------------------------------------

    Controllers::ViewController*    viewController() const;
    Controllers::DataController*    dataController() const;
    Controllers::ProjectController* projectController() const;
    Controllers::VideoController*   videoController() const;

  private:
    // контроллеры управляющие разными областями программы (менеджерами)
    std::unique_ptr<Controllers::ViewController>    m_viewController;
    std::unique_ptr<Controllers::DataController>    m_dataController;
    std::unique_ptr<Controllers::ProjectController> m_projectController;
    std::unique_ptr<Controllers::VideoController>   m_videoController;

    // менеджеры отвечающие за отдельные части системы
    std::unique_ptr<TaskManager> m_taskManager;
    // std::shared_ptr<VideoExportManager> m_videoExportManager;
    std::unique_ptr<ObjectRegistry>     m_objectRegistry;
    std::unique_ptr<DataManager>        m_dataManager;
    std::unique_ptr<ViewManager>        m_viewManager;
    std::unique_ptr<LayerManager>       m_layerManager;
    std::unique_ptr<SessionManager>     m_sessionManager;
    std::unique_ptr<FileSchemeRegistry> m_fileSchemeRegistry;
    QSpace::Session::CurrentSession     m_session_state;

    // модели данных
    std::unique_ptr<Models::DataTreeModel> m_dataTreeModel;
};
} // namespace QSpace::Core