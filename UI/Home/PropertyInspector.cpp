#include "PropertyInspector.h"
#include "ColorMapComboBox.h"
#include "ColorMapEditorDialog.h"
#include "Common/Enums/LayerEnums.h"
#include "Core/AppCore/AppCore.h"
#include "Core/AppCore/DataController.h"
#include "Core/AppCore/ProjectController.h"
#include "SettingsEditorWidget.h"
#include "Structures/ColormapPresets.h"
#include "Visualize/ColorMapManager/ColorMapManager.h"
#include "ui_PropertyInspector.h"
#include <QSignalBlocker>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qcustomplot.h>
#include <qelapsedtimer.h>
#include <qicon.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qoverload.h>
#include <qslider.h>
#include <qspinbox.h>
#include <quuid.h>
#include <qvariant.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>

namespace QSpace::UI {
PropertyInspector::PropertyInspector(Core::AppCore* app, QWidget* parent)
    : QWidget(parent), ui(new Ui::PropertyInspector), m_app(app) {
    ui->setupUi(this);
    ui->colorbarWidget->setColorMap(QSpace::Visualize::ColorMapPresets::getPresetByName("Plasma"));
    m_dynamicEditor = new SettingsEditorWidget(this);
    if (ui->scrollArea_DynamicProperties) {
        // Если у ScrollArea еще нет внутреннего виджета-контейнера, создаем его
        if (!ui->scrollArea_DynamicProperties->widget()) {
            QWidget* container = new QWidget();
            container->setLayout(new QVBoxLayout());
            ui->scrollArea_DynamicProperties->setWidget(container);
            ui->scrollArea_DynamicProperties->setWidgetResizable(true);
        }
        // Добавляем наш инспектор в компоновщик scroll area
        ui->scrollArea_DynamicProperties->widget()->layout()->addWidget(m_dynamicEditor);
    } else if (this->layout()) {
        // Фолбэк: если scroll area нет, просто кидаем в самый низ главного layout'а панели
        this->layout()->addWidget(m_dynamicEditor);
    }
    // Выключаем панель, пока ничего не выбрано
    this->setEnabled(false);
    setupUiLogic();
}

PropertyInspector::~PropertyInspector() {
    delete ui;
}

void PropertyInspector::setupUiLogic() {
    // 1. Заполняем палитры
    ui->combo_colormap->clear();
    ui->combo_colormap->setIconSize(QSize(50, 18));
    auto& manager = Visualize::ColorMapManager::instance();
    for (auto colorMap : manager.getAllMaps()) {
        QIcon icon = manager.createColorMapIcon(colorMap);
        ui->combo_colormap->addItem(icon,          // палитра
                                    colorMap.name, // название
                                    colorMap.id    // Прячем Enum в UserData
        );
    }

    //  2. Коннектим сигналы UI к слотамв
    connect(ui->combo_colormap,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &PropertyInspector::handlePaletteChange);
    connect(ui->combo_colormap,
            &ColorMapComboBox::newButtonClicked,
            this,
            &PropertyInspector::handleCreateCustomPalete);
    connect(ui->combo_colormap,
            &ColorMapComboBox::editButtonClicked,
            this,
            &PropertyInspector::handleEditPalete);

    connect(ui->combo_current_column,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &PropertyInspector::handleCurrentVisualizeColumnChange);
    connect(&Visualize::ColorMapManager::instance(),
            &Visualize::ColorMapManager::paleteAdded,
            this,
            &PropertyInspector::handleColorMapAdded);
    connect(ui->checkBox_colorbarOrientation, &QCheckBox::toggled, [this](bool isChecked) {
        ui->colorbarWidget->setOrientation(isChecked);
    });
}

void PropertyInspector::handleColorMapAdded(const Visualize::ColorMap& map) {
    auto& manager = Visualize::ColorMapManager::instance();
    QIcon icon    = manager.createColorMapIcon(map);
    ui->combo_colormap->removeItem(
        ui->combo_colormap->findData(map.id)); // Удаляем старую запись, если она есть
    ui->combo_colormap->addItem(icon, map.name, map.id);
    // 6. Обновляем комбобокс без срабатывания сигналов
    QSignalBlocker blocker(ui->combo_colormap);
    // Выбираем свежедобавленную палитру
    ui->combo_colormap->setCurrentIndex(ui->combo_colormap->count() - 1);
    // 7. Обновляем ядро и рендер
    auto colorMapId = map.id;
    auto layer      = m_app->dataController()->getLayerById(m_currentElementId);
    layer->getSettings()->setColorMapId(colorMapId);
}
void PropertyInspector::handleCreateCustomPalete() {
    if (m_currentNodeId.isNull())
        return;

    // 1. Берем текущую палитру как базовую (чтобы было что редактировать)
    auto layer      = m_app->dataController()->getLayerById(m_currentElementId);
    auto paleteId   = layer->getSettings()->colorMapId();
    auto baseMapOpt = QSpace::Visualize::ColorMapManager::instance().getMap(paleteId);
    if (!baseMapOpt.has_value()) {
        QMessageBox::warning(this, "Ошибка", "Текущая палитра не найдена. Невозможно создать на ее основе.");
        return;
    }
    // Если палитра не найдена, берем первую дефолтную
    Visualize::ColorMap baseMap = baseMapOpt.has_value()
                                      ? baseMapOpt.value()
                                      : Visualize::ColorMapManager::instance().getAllMaps().first();

    // 2. Создаем диалог, передавая базовую палитру
    ColorMapEditorDialog dialog(baseMap, this);
    connect(&dialog,
            &ColorMapEditorDialog::savePaletteRequested,
            m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::savePalette);
    connect(&dialog,
            &ColorMapEditorDialog::loadPaletteRequested,
            m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::loadPalette);
    connect(m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::paletteLoaded,
            &dialog,
            &ColorMapEditorDialog::onPaletteLoaded);
    // 3. Открываем окно и ждем. Выполнение кода здесь останавливается,
    // пока пользователь не нажмет Ok или Cancel
    if (dialog.exec() == QDialog::Accepted) {
        // 4. Забираем готовую палитру из окна
        Visualize::ColorMap customMap = dialog.getEditedMap();

        // 5. Сохраняем в менеджер
        auto& manager = QSpace::Visualize::ColorMapManager::instance();
        manager.AddCustomMap(customMap);
    }
}
void PropertyInspector::handleEditPalete() {
    if (m_currentNodeId.isNull())
        return;

    auto layer      = m_app->dataController()->getLayerById(m_currentElementId);
    auto nodeId     = layer->getSettings()->colorMapId();
    auto baseMapOpt = QSpace::Visualize::ColorMapManager::instance().getMap(nodeId);

    if (!baseMapOpt.has_value()) {
        QMessageBox::warning(this, "Ошибка", "Текущая палитра не найдена. Невозможно отредактировать.");
        return;
    }
    if (baseMapOpt.value().isPreset) {
        QMessageBox::warning(this,
                             "Ошибка",
                             "Невозможно редактировать пресет. Создайте копию палитры и редактируйте ее.");
        return;
    }
    Visualize::ColorMap baseMap = baseMapOpt.value();

    ColorMapEditorDialog dialog(baseMap, this);
    connect(&dialog,
            &ColorMapEditorDialog::savePaletteRequested,
            m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::savePalette);
    connect(&dialog,
            &ColorMapEditorDialog::loadPaletteRequested,
            m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::loadPalette);
    connect(m_app->projectController(),
            &QSpace::Core::Controllers::ProjectController::paletteLoaded,
            &dialog,
            &ColorMapEditorDialog::onPaletteLoaded);
    if (dialog.exec() == QDialog::Accepted) {
        Visualize::ColorMap editedMap = dialog.getEditedMap();
        // Если палитра была загружена из файла, сохраняем ее ID и имя
        editedMap.id       = baseMap.id;
        editedMap.name     = baseMap.name;
        editedMap.isPreset = false;

        auto& manager = QSpace::Visualize::ColorMapManager::instance();
        manager.AddCustomMap(editedMap);
    }
}

void PropertyInspector::handleCurrentVisualizeColumnChange(int index) {
    if (m_currentNodeId.isNull())
        return;

    QString fieldName = ui->combo_current_column->itemText(index);
    auto    layer     = m_app->dataController()->getLayerById(m_currentElementId);
    layer->getSettings()->setColorByField(fieldName);
    updateWidgets();
}

void PropertyInspector::setCurrentElement(const QUuid& id, Models::DataTreeItem::Type type) {
    // Если id пустой или выбран не слой — блокируем панель и сбрасываем id
    if (id.isNull() || type != Models::DataTreeItem::Type::LayerItem) {
        // m_currentNodeId = QUuid();
        // this->setEnabled(false);
        // m_dynamicEditor->setSettings(nullptr);
        return;
    }
    // Сюда доходим только если выбран корректный слой
    auto layer = m_app->dataController()->getLayerById(id);
    if (layer) {
        m_currentElementId = layer->layerId();
        m_currentNodeId    = layer->dataNodeId();
        this->setEnabled(true);
    }
    updateWidgets();
}
void PropertyInspector::updateWidgets() {
    auto node  = m_app->dataController()->getNodeById(m_currentNodeId);
    auto layer = m_app->dataController()->getLayerById(m_currentElementId);
    if (!node || !layer)
        return;

    // Блокируем сигналы, чтобы установка значений в UI не вызвала обратную команду в Core
    QSignalBlocker blocker(this);
    QSignalBlocker comboBlocker(ui->combo_colormap);
    QSignalBlocker comboBlocker1(ui->combo_current_column);
    if (node->data) {
        ui->combo_current_column->clear();
        auto pointData = node->data->GetPointData();
        int  numArrays = pointData->GetNumberOfArrays();
        for (int i = 0; i < numArrays; ++i) {
            vtkDataArray* array = pointData->GetArray(i);
            if (array) {
                QString arrayName = array->GetName();
                if (!arrayName.isEmpty()) {
                    ui->combo_current_column->addItem(arrayName);
                }
            }
        }
    }
    int index;
    // Устанавливаем текущее выбранное поле из настроек ноды
    index = ui->combo_current_column->findText(layer->getSettings()->colorByField());
    if (index != -1) {
        ui->combo_current_column->setCurrentIndex(index);
    }
    // Устанавливаем текущую палитру в комбобоксе
    index = ui->combo_colormap->findData(QVariant::fromValue(layer->getSettings()->colorMapId()));
    if (index != -1) {
        ui->combo_colormap->setCurrentIndex(index);
        handlePaletteChange(index);
    }

    m_dynamicEditor->setSettings(layer->getSettings().get());
    ui->colorbarWidget->setSettings(layer->getSettings().get());
}

void PropertyInspector::handlePaletteChange(int index) {
    if (m_currentNodeId.isNull())
        return;

    // Достаем тип палитры из выбранного пункта
    auto colorMapId   = ui->combo_colormap->itemData(index).value<QUuid>();
    auto layer        = m_app->dataController()->getLayerById(m_currentElementId);
    auto colorMap_ptr = QSpace::Visualize::ColorMapManager::instance().getMap(colorMapId);
    if (colorMap_ptr.has_value()) {
        ui->colorbarWidget->setColorMap(colorMap_ptr.value());
    }
    layer->getSettings()->setColorMapId(colorMapId);
    layer->getView().lock()->render();
}

} // namespace QSpace::UI