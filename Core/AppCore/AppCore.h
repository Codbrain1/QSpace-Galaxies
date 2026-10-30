#pragma once
#include <QObject>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <quuid.h>
#include <vtkRenderWindow.h>
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
} // namespace QSpace::Core

namespace QSpace::Models {
class DataTreeModel;
} // namespace QSpace::Models

// Forward declarations Контроллеров
namespace QSpace::Core::Controllers {
class ViewController;
class ProjectController;
class VideoController;
class ImportController;
class LayerController;
class ExperimentController;
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
    DataManager*            dataManager() const;

    // ---------------------------------------------------------
    // @SECTION: контроллеры
    // ---------------------------------------------------------

    Controllers::ViewController*    viewController() const;
    Controllers::ImportController*  importController() const;
    Controllers::LayerController*      layerController() const;
    Controllers::ExperimentController* experimentController() const;
    Controllers::ProjectController*    projectController() const;
    Controllers::VideoController*   videoController() const;

  private:
    // контроллеры управляющие разными областями программы (менеджерами)
    std::unique_ptr<Controllers::ViewController>    m_viewController;
    std::unique_ptr<Controllers::ImportController>  m_importController;
    std::unique_ptr<Controllers::LayerController>      m_layerController;
    std::unique_ptr<Controllers::ExperimentController> m_experimentController;
    std::unique_ptr<Controllers::ProjectController>    m_projectController;
    std::unique_ptr<Controllers::VideoController>   m_videoController;

    // менеджеры отвечающие за отдельные части системы
    std::unique_ptr<TaskManager>        m_taskManager;
    std::unique_ptr<ObjectRegistry>     m_objectRegistry;
    std::unique_ptr<DataManager>        m_dataManager;
    std::unique_ptr<ViewManager>        m_viewManager;
    std::unique_ptr<LayerManager>       m_layerManager;
    std::unique_ptr<SessionManager>     m_sessionManager;
    std::unique_ptr<FileSchemeRegistry> m_fileSchemeRegistry;

    // модели данных
    std::unique_ptr<Models::DataTreeModel> m_dataTreeModel;
};
} // namespace QSpace::Core