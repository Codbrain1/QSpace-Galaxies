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
#include "ImportController.h"
#include "LayerController.h"
#include "ExperimentController.h"
#include "Models/DataTreeModel/DataTreeModel.h"
#include "ProjectController.h"
#include "VideoController.h"
#include "ViewController.h"

namespace QSpace::Core {
AppCore::AppCore(QObject* parent) : QObject(parent) {
    // 1. Инициализация менеджеров (Базовый слой)
    m_taskManager        = std::make_unique<TaskManager>();
    m_objectRegistry     = std::make_unique<ObjectRegistry>();
    m_dataManager        = std::make_unique<DataManager>(m_taskManager.get());
    m_viewManager        = std::make_unique<ViewManager>();
    m_layerManager       = std::make_unique<LayerManager>();
    m_sessionManager     = std::make_unique<SessionManager>(m_objectRegistry.get(),
                                                            m_layerManager.get(),
                                                            m_viewManager.get());
    m_fileSchemeRegistry = std::make_unique<FileSchemeRegistry>();

    // 2. Инициализация контроллеров (Внедрение зависимостей)
    m_viewController = std::make_unique<Controllers::ViewController>(m_viewManager.get(),
                                                                     m_layerManager.get(),
                                                                     m_objectRegistry.get());

    m_importController = std::make_unique<Controllers::ImportController>(m_dataManager.get(),
                                                                          m_objectRegistry.get(),
                                                                          m_fileSchemeRegistry.get());
    m_layerController  = std::make_unique<Controllers::LayerController>(m_objectRegistry.get(),
                                                                       m_layerManager.get(),
                                                                       m_viewManager.get());
    m_experimentController = std::make_unique<Controllers::ExperimentController>(
        m_objectRegistry.get(), m_layerManager.get(), m_viewManager.get());

    m_projectController =
        std::make_unique<Controllers::ProjectController>(m_sessionManager.get(),
                                                         m_objectRegistry.get(),
                                                         m_dataManager.get(),
                                                         m_importController.get());

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
    m_importController->initialize();
    m_projectController->initialize();
    connect(m_importController.get(),
            &Controllers::ImportController::nodeImported,
            m_layerController.get(),
            &Controllers::LayerController::createDefaultLayers);
    connect(m_importController.get(),
            &Controllers::ImportController::nodeDataLoaded,
            m_layerController.get(),
            &Controllers::LayerController::onNodeDataLoaded);
    connect(m_experimentController.get(),
            &Controllers::ExperimentController::markSessionDirty,
            m_importController.get(),
            &Controllers::ImportController::markSessionDirty);
    connect(m_layerController.get(),
            &Controllers::LayerController::sceneUpdateRequested,
            m_viewController.get(),
            &Controllers::ViewController::sceneUpdateRequested,
            Qt::QueuedConnection);
    connect(m_layerController.get(),
            &Controllers::LayerController::markSessionDirty,
            m_importController.get(),
            &Controllers::ImportController::markSessionDirty);

    m_videoController->initialize();
    connect(m_videoController.get(),
            &Controllers::VideoController::requestNodeLoad,
            m_importController.get(),
            &Controllers::ImportController::onRequestDataLoad,
            Qt::QueuedConnection);
    connect(m_importController.get(),
            &Controllers::ImportController::nodeDataLoaded,
            m_videoController.get(),
            &Controllers::VideoController::handleNodeDataLoaded,
            Qt::QueuedConnection);

    // регистрируем стандартные схемы для менеджера схем файлов
    m_fileSchemeRegistry->registerStandartPresets();
}

Controllers::ViewController* AppCore::viewController() const {
    return m_viewController.get();
}

Controllers::ImportController* AppCore::importController() const {
    return m_importController.get();
}

Controllers::LayerController* AppCore::layerController() const {
    return m_layerController.get();
}

Controllers::ExperimentController* AppCore::experimentController() const {
    return m_experimentController.get();
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

DataManager* AppCore::dataManager() const {
    return m_dataManager.get();
}

} // namespace QSpace::Core