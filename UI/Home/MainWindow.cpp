#include "MainWindow.h"
#include "Common/Logger/Logger.h"
#include "Core/AppCore/AppCore.h"
#include "Core/AppCore/DataController.h"
#include "Core/AppCore/ProjectController.h"
#include "Core/AppCore/VideoController.h"
#include "Core/AppCore/ViewController.h"
#include "Enums/ViewEnums.h"
#include "Enums/VisualizeBaseEnums.h"
#include "LayerExplorerWidget.h"
#include "Models/DataTreeModel/DataTreeModel.h"
#include "PropertyInspector.h"
#include "TimeLineWidget.h"
#include "ViewContainerWidget.h"
#include "Visualize/Views/View3D/AbstractView3D.h"
#include "ui_newmainwindow.h"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVTKOpenGLNativeWidget.h>
#include <QWidgetAction>
#include <memory>
#include <qaction.h>
#include <qcombobox.h>
#include <qcontainerfwd.h>
#include <qdockwidget.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qmainwindow.h>
#include <qmenu.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qsharedpointer.h>
#include <qtoolbutton.h>
#include <quuid.h>
#include <vtkDataSetAttributes.h>
#include <vtkType.h>

namespace QSpace::UI {
MainWindow::MainWindow(Core::AppCore* app, QWidget* parent)
    : QMainWindow(parent), m_app(app), ui(new Ui::MainWindow) {
    // инициализируем ui файл
    ui->setupUi(this);
    setDockNestingEnabled(true);
    this->setCentralWidget(nullptr);

    // Вызываем инициализацию ядра
    m_app->initialize();

    // инициализируем меню слоев LayerExplorerWidget
    //--------------------------------------------------
    m_layerExplorerWidget = std::make_unique<LayerExplorerWidget>(app);
    ui->dockWidget_LayerExplorer->setWidget(m_layerExplorerWidget.get());
    // инициализируем меню настроек слоя PropertyInspector
    //--------------------------------------------------
    m_propertyInspector = std::make_unique<PropertyInspector>(app);
    ui->dock_properties->setWidget(m_propertyInspector.get());
    ui->dock_properties->setVisible(false);
    // инициализация Timeslider
    //--------------------------------------------------
    m_timeSliderWidget = std::make_unique<TimeLineWidget>(app);
    ui->dock_timeSlider->setWidget(m_timeSliderWidget.get());
    ui->dock_timeSlider->setVisible(true);
    // --- ПОДГОТОВКА ДИАЛОГА ПРОГРЕССА ---
    //--------------------------------------------------

    // m_exportProgressDialog =
    //     std::make_unique<QProgressDialog>(tr("Video Rendering..."), tr("Cancel"), 0, 100, this);
    // m_exportProgressDialog->setWindowTitle(tr("Exporting Video"));
    // m_exportProgressDialog->setWindowModality(Qt::WindowModal); // Блокируем главное окно
    // m_exportProgressDialog->setAutoClose(true);
    // m_exportProgressDialog->setAutoReset(true);
    // m_exportProgressDialog->reset(); // Скрываем по умолчанию

    {
        using namespace QSpace::Visualize::Views::View3D;
        cameraComboBox = new QComboBox(this);
        cameraComboBox->addItem("Сверху", QVariant::fromValue(CameraViewType::XY_Top));
        cameraComboBox->addItem("Спереди", QVariant::fromValue(CameraViewType::XZ_Front));
        cameraComboBox->addItem("Справа", QVariant::fromValue(CameraViewType::YZ_Right));
        cameraComboBox->addItem("Изометрия", QVariant::fromValue(CameraViewType::Iso));
        ui->mainToolBar->insertWidget(ui->action_bg_settings, cameraComboBox);

        // 4. Добавляем небольшой визуальный отступ (чтобы не слипалось с соседней кнопкой)
        ui->mainToolBar->insertSeparator(ui->action_bg_settings);
    }
    if (auto* btn = qobject_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->action_bg_settings))) {
        btn->setMenu(ui->menu_bg); // Берем уже готовое меню из UI
        btn->setPopupMode(QToolButton::InstantPopup);
    }

    // подключаем слоты
    setupSlots();
    m_app->viewController()->createView(Visualize::Views::ViewType::OpenGL3D);
    // Добавьте это в самый конец конструктора
    // Сначала явно перемещаем док
    addDockWidget(Qt::BottomDockWidgetArea, ui->dock_timeSlider);

    // Принудительно "разделяем" область, чтобы слайдер был снизу,
    // а другие доки не могли залезть в его зону
    // Если LayerExplorer слева, мы принудительно отделяем низ от левой части
    splitDockWidget(ui->dockWidget_LayerExplorer, ui->dock_timeSlider, Qt::Vertical);

    // Устанавливаем приоритет углов (оставляем, как было)
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
}

void MainWindow::handleViewCreated(const QUuid& viewId, Visualize::Views::ViewType type) {
    if (viewId.isNull()) {
        qCCritical(LogUI) << "Failed to create view!";
        return;
    }

    auto view       = m_app->viewController()->getView(viewId);
    auto viewWidget = view->getWidget();

    if (!view || !viewWidget) {
        qCCritical(LogUI) << "View created but not found in AppCore!";
        return;
    }

    QString      dockTitle = tr("View - %1").arg(view->viewName());
    QDockWidget* dock      = new QDockWidget(dockTitle, this);
    dock->setObjectName(viewId.toString()); // Устанавливаем имя для поиска при удалении
    // стандартное поведение дока
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);
    dock->setWidget(viewWidget);

    if (auto view3D = dynamic_cast<QSpace::Visualize::Views::AbstractView3D*>(view)) {
        auto* container = new UI::ViewContainerWidget(viewWidget, view3D->sceneSettings(), dock);
        dock->setWidget(container);
    }
    // Используем лямбду для передачи viewId
    connect(dock, &QDockWidget::destroyed, this, [this, viewId]() {
        //   m_viewDockWidgets.remove(viewId); QMetaObject::invokeMethod(m_app,
        //   "requestViewRemoval",
        //   Qt::QueuedConnection,
        //   Q_ARG(QUuid, viewId));
    });
    // 5. Размещение дока на форме
    // Если это не первое окно — создаем вкладки (Tabbed Layout)
    if (!m_viewDockWidgets.isEmpty()) {
        // Берем любое существующее окно и группируем новое с ним
        tabifyDockWidget(m_viewDockWidgets.values().first(), dock);
    } else {
        // Если окон еще нет, прижимаем вправо
        splitDockWidget(ui->dockWidget_LayerExplorer, dock, Qt::Horizontal);
        resizeDocks({ui->dockWidget_LayerExplorer, dock, ui->dock_properties},
                    {250, 600, 300}, // Желаемые ширины в пикселях
                    Qt::Horizontal);
    }
    // Сохраняем в карту для управления жизненным циклом
    m_viewDockWidgets.insert(viewId, dock);

    // Выводим на передний план
    dock->show();
    dock->raise();
}

void MainWindow::handleViewRemoved(const QUuid& viewId) {
    if (!m_viewDockWidgets.contains(viewId))
        return;

    // Извлекаем указатель из мапы
    QDockWidget* dock = m_viewDockWidgets.take(viewId);

    if (dock) {
        // 1. Отключаем виджет от дока.
        // ЭТО ВАЖНО: Если этого не сделать, деструктор QDockWidget
        // попытается удалить renderWidget. Но им владеет ViewManager!
        // Обнуление предотвращает double-free crash.
        dock->setWidget(nullptr);

        // 2. Убираем из интерфейса
        removeDockWidget(dock);

        // 3. Планируем безопасное удаление самого объекта дока
        dock->deleteLater();
    }
}
void MainWindow::setupSlots() {
    // обработчик снизу вверх, например если потребуется синхронизировать несколько окон (одновременно
    // крутить)
    connect(m_app->viewController(),
            &QSpace::Core::Controllers::ViewController::sceneUpdateRequested,
            this,
            &MainWindow::handleRenderUpdate,
            Qt::QueuedConnection);
    connect(m_app->dataController(),
            &QSpace::Core::Controllers::DataController::sceneUpdateRequested,
            this,
            &UI::MainWindow::handleRenderUpdate,
            Qt::QueuedConnection);
    connect(m_app->dataTreeModel(),
            &QSpace::Models::DataTreeModel::sceneUpdateRequested,
            this,
            &UI::MainWindow::handleRenderUpdate,
            Qt::QueuedConnection);
    //----- ПОДКЛЮЧЕНИЕ СЛОТОТОВ LayerExplorerWidget -----

    // ---- Подключение кнопок QToolBar ----
    // работа со сценой
    connect(ui->action_reset_camera, &QAction::triggered, this, &MainWindow::resetCamera);
    connect(ui->action_toggle_axes, &QAction::toggled, this, &MainWindow::axesVisibleToggled);
    connect(ui->action_toggle_grid, &QAction::toggled, this, &MainWindow::gridVisibleToggled);

    // // запуск рендеринга видео
    // QAction* actionExport =
    //     ui->mainToolBar->addAction(QIcon::fromTheme("video-x-generic"), tr("Export Video"));
    // connect(actionExport, &QAction::triggered, this, &MainWindow::handleVideoExport);

    // загрузка и сохранение проекта
    connect(ui->action_save_session, &QAction::triggered, this, &MainWindow::handleProjectSave);
    connect(ui->action_open_session, &QAction::triggered, this, &MainWindow::handleProjectOpen);

    // --- СОЗДАНИЕ АНИМАЦИИ ---
    // connect(m_exportProgressDialog.get(),
    //         &QProgressDialog::canceled,
    //         m_app->videoController(),
    //         &Core::Controllers::VideoController::cancelVideoExport);
    // connect(m_app->videoController(),
    //         &Core::Controllers::VideoController::exportProgressUpdated,
    //         this,
    //         [this](int current, int total) {
    //             m_exportProgressDialog->setMaximum(total);
    //             m_exportProgressDialog->setValue(current);
    //         });
    // connect(m_app->videoController(),
    //         &Core::Controllers::VideoController::exportFinished,
    //         this,
    //         &MainWindow::handleExportFinished);

    connect(m_layerExplorerWidget.get(),
            &LayerExplorerWidget::selectionChanged,
            this,
            &MainWindow::handleLayerSelectionsChange);
    connect(m_timeSliderWidget.get(),
            &TimeLineWidget::editLayerProperty,
            this,
            &MainWindow::handleLayerSelectionChange);

    connect(cameraComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::handleCameraViewChange);
    connect(ui->menu_bg, &QMenu::triggered, this, &MainWindow::handleBackgroundChange);

    connect(m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::requestSavePathFromUI,
            this,
            &MainWindow::handleSavePathSelection);

    // 2. Обновление заголовка окна при изменении состояния сессии
    connect(m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::sessionStateChanged,
            this,
            &MainWindow::handleSessionStateChange);

    // ---- РАБОТА С ОКНАМИ ----
    connect(m_app->viewController(),
            &QSpace::Core::Controllers::ViewController::viewCreated,
            this,
            &MainWindow::handleViewCreated);
    connect(m_app->viewController(),
            &QSpace::Core::Controllers::ViewController::viewRemoved,
            this,
            &MainWindow::handleViewRemoved);
    connect(m_layerExplorerWidget.get(),
            &LayerExplorerWidget::propertyInspectorVisibleRequested,
            this,
            [this](const bool isVisible) { ui->dock_properties->setVisible(isVisible); });
    connect(
        m_layerExplorerWidget.get(),
        &LayerExplorerWidget::layerStructureChanged,
        this,
        [this](Models::DataTreeModel::TreeMode mode) {
            if (mode == Models::DataTreeModel::TreeMode::ComponentView) {
                ui->dock_timeSlider->setVisible(false);
            } else {
                ui->dock_timeSlider->setVisible(true);
            }
        },
        Qt::ConnectionType::QueuedConnection);
    connect(ui->action_exit, &QAction::triggered, qApp, &QCoreApplication::quit);
}
void MainWindow::handleLayerSelectionsChange(const QList<LayerExplorerWidget::SelectedItem>& ids) {
    if (ids.isEmpty()) {
        m_propertyInspector->setCurrentElement(QUuid(), Models::DataTreeItem::Type::Root);
    } else {
        // Берем первый выбранный элемент и передаем в инспектор
        m_propertyInspector->setCurrentElement(ids.first().id, ids.first().type);
    }
}
void MainWindow::handleLayerSelectionChange(const QUuid& layerId) {
    m_propertyInspector->setCurrentElement(layerId, Models::DataTreeItem::Type::LayerItem);
}
// void MainWindow::handleExportFinished(bool success) {
//     m_exportProgressDialog->reset(); // Прячем окно
//     if (success) {
//         QMessageBox::information(this, tr("Ready"), tr("Video saved successfully!"));
//     } else {
//         QMessageBox::warning(this, tr("Cancel"), tr("Video export was canceled or finished with an
//         error."));
//     }
// }
void MainWindow::handleSessionStateChange(const QSpace::Session::CurrentSession& session) {
    QString title = QString("QSpace - %1%2")
                        .arg(session.projectName.isEmpty() ? tr("New Project") : session.projectName)
                        .arg(session.isDirty ? "*" : ""); // Звездочка, если есть несохраненные изменения
    setWindowTitle(title);

    qCDebug(LogSystem) << "Session state updated. Project:" << session.projectName;
}
void MainWindow::handleSavePathSelection() {
    QString path =
        QFileDialog::getSaveFileName(this, tr("Save project as..."), "", tr("QSpace Project (*.qsp)"));
    if (!path.isEmpty()) {
        // Устанавливаем путь в стейт и пробуем сохранить еще раз
        m_app->projectController()->saveCurrentProjectAs(path);
    }
}
// void MainWindow::handleVideoExport() {
//     // 1. Проверяем, выбран ли слой, который будем анимировать
//     auto selectedItems = m_layerExplorerWidget->getSelectedIds();
//     if (selectedItems.isEmpty()) {
//         QMessageBox::warning(this,
//                              tr("No layer selected"),
//                              tr("Please select a layer in the tree to export an animation based on it."));
//         return;
//     }
//     QUuid baseNodeId =
//         selectedItems.first().id; // Для простоты берем первый выбранный слой. Можно расширить логику
//         позже.

//     // 2. Выбираем файлы для анимации
//     QStringList files = QFileDialog::getOpenFileNames(this,
//                                                       tr("Select data files for animation"),
//                                                       "",
//                                                       tr("Bin Files (*.bin)"));
//     if (files.isEmpty())
//         return;

//     // 3. Выбираем куда сохранить видео
//     QString savePath =
//         QFileDialog::getSaveFileName(this, tr("Save video as..."), "", tr("Video Files (*.ogv)"));
//     if (savePath.isEmpty())
//         return;

//     // TODO: Здесь можно добавить вызов QInputDialog для запроса `stride` (шага кадров) у
//     // пользователя
//     int stride = 1;
//     int fps    = 16;
//     // 4. Показываем диалог загрузки и запускаем процесс
//     m_exportProgressDialog->setValue(0);
//     m_exportProgressDialog->show();

//     // m_app->videoController()->startVideoExport(baseNodeId, files, savePath, stride, fps);
// }
MainWindow::~MainWindow() {
    delete ui;
}
void MainWindow::resetCamera() {
    m_app->viewController()->resetCameraInAllViews();
}
void MainWindow::handleCameraViewChange(int index) {
    // Меняем камеру
    auto viewType = cameraComboBox->itemData(index).value<Visualize::Views::View3D::CameraViewType>();
    m_app->viewController()->setCameraViewInAllViews(viewType);
}
void MainWindow::handleBackgroundChange(QAction* action) {
    if (!action)
        return;

    // Смена цвета в рендерере
    if (action == ui->action_bg_black) {
        m_app->viewController()->setBackgroundColorInAllViews(0.0, 0.0, 0.0);
    } else if (action == ui->action_bg_white) {
        m_app->viewController()->setBackgroundColorInAllViews(1.0, 1.0, 1.0);
    }

    // Меняем текст на кнопке тулбара
    ui->action_bg_settings->setText(action->text());
    ui->action_bg_settings->setIcon(action->icon());
}
void MainWindow::axesVisibleToggled(bool visible) {
    m_app->viewController()->setAxesVisibleInAllViews(visible);
}
void MainWindow::gridVisibleToggled(bool visible) {
    m_app->viewController()->setGridVisibleInAllViews(visible);
}
// ------------------------- слоты обработка действий пользователя ------------------------------
void MainWindow::handleRenderUpdate() {
    m_app->viewController()->getViewManager()->renderAllViews();
}

void MainWindow::handleProjectSave() {
    m_app->projectController()->saveCurrentProject();
}

void MainWindow::handleProjectOpen() {
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open project..."),
                                                "",
                                                tr("QSpace Project (*.qsp);;All Files (*)"));

    if (!path.isEmpty()) {
        m_app->projectController()->openProject(path);
    }
}
} // namespace QSpace::UI