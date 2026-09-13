#include "AppCore.h"
#include "Core/DataManager/DataManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/TaskManager/TaskManager.h"
#include <memory>

#include "Core/DataManager/DataManager.h"
#include "Core/FileSchemeRegistry/FileSchemeRegistry.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/SessionManager/SessionManager.h"
#include "Core/TaskManager/TaskManager.h"
#include "Core/ViewManager/ViewManager.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <qfileinfo.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <quuid.h>
#include "DataController.h"
#include "Models/DataTreeModel/DataTreeModel.h"
#include "ProjectController.h"
#include "VideoController.h"
#include "ViewController.h"

namespace QSpace::Core {
AppCore::AppCore(QObject* parent) : QObject(parent) {
    // 1. Инициализация менеджеров (Базовый слой)
    m_taskManager    = std::make_unique<TaskManager>();
    m_objectRegistry = std::make_unique<ObjectRegistry>();
    m_dataManager    = std::make_unique<DataManager>(m_taskManager.get());
    m_viewManager    = std::make_unique<ViewManager>();
    m_layerManager   = std::make_unique<LayerManager>();
    m_sessionManager =
        std::make_unique<SessionManager>(m_objectRegistry.get(), m_layerManager.get());
    m_fileSchemeRegistry = std::make_unique<FileSchemeRegistry>();

    // 2. Инициализация контроллеров (Внедрение зависимостей)
    m_viewController = std::make_unique<Controllers::ViewController>(m_viewManager.get(),
                                                                     m_layerManager.get(),
                                                                     m_objectRegistry.get());

    m_dataController = std::make_unique<Controllers::DataController>(m_dataManager.get(),
                                                                     m_objectRegistry.get(),
                                                                     m_layerManager.get(),
                                                                     m_viewManager.get());

    m_projectController = std::make_unique<Controllers::ProjectController>(m_sessionManager.get(),
                                                                           m_objectRegistry.get(),
                                                                           m_dataManager.get(),
                                                                           m_dataController.get());

    m_videoController = std::make_unique<Controllers::VideoController>(m_objectRegistry.get(),
                                                                       m_viewManager.get(),
                                                                       m_layerManager.get());
    // 3. Инициализация моделей данных
    m_dataTreeModel = std::make_unique<QSpace::Models::DataTreeModel>(m_objectRegistry.get(),
                                                                      m_layerManager.get());
}

AppCore::~AppCore() = default;

void AppCore::initialize() {
    m_viewController->initialize();
    m_dataController->initialize();
    m_projectController->initialize();
    m_videoController->initialize();
    connect(m_videoController.get(),
            &Controllers::VideoController::requestNodeLoad,
            m_dataController.get(),
            &Controllers::DataController::onRequestDataLoad,
            Qt::QueuedConnection);

    // регистрируем стандартные схемы для менеджера схем файлов
    m_fileSchemeRegistry->registerStandartPresets();
}

Controllers::ViewController* AppCore::viewController() const {
    return m_viewController.get();
}

Controllers::DataController* AppCore::dataController() const {
    return m_dataController.get();
}

Controllers::ProjectController* AppCore::projectController() const {
    return m_projectController.get();
}

Controllers::VideoController* AppCore::videoController() const {
    return m_videoController.get();
}

Models::DataTreeModel* AppCore::dataTreeModel() const {
    return m_dataTreeModel.get();
}

} // namespace QSpace::Core