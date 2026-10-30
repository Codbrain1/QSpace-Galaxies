#include "TimeLineWidget.h"
#include "Common/Logger/Logger.h"
#include "Core/AppCore/ImportController.h"
#include "Core/AppCore/LayerController.h"
#include "Core/AppCore/VideoController.h"
#include "QAbstractItemView"
#include "ui_TimeLineWidget.h"
#include <qloggingcategory.h>
#include <qtoolbutton.h>

namespace QSpace::UI {

TimeLineWidget::TimeLineWidget(Core::AppCore* app, QWidget* parent)
    : QWidget(parent), ui(new Ui::TimeLineWidget), m_app(app) {
    ui->setupUi(this);

    connect(m_app->videoController(),
            &QSpace::Core::Controllers::VideoController::snapshotsListSizeChanged,
            this,
            &TimeLineWidget::handleSnapshotsListChangeSize);
    connect(this,
            &TimeLineWidget::currentTimeStampValueChanged,
            m_app->videoController(),
            &QSpace::Core::Controllers::VideoController::handleActivateVisualizeSnapshot);
    // 1. Обработка изменения значения (перетаскивание или программный ввод)
    connect(ui->TimeSlider,
            &QSlider::valueChanged,
            this,
            &TimeLineWidget::handleTimeSliderChangeValue); // TODO: транслировать сигнал qt на собственный
                                                           // сигнал класса

    connect(ui->toolButtonPrev, &QToolButton::clicked, this, &TimeLineWidget::handleToolButtonPrev_clicked);
    connect(ui->toolButtonNext, &QToolButton::clicked, this, &TimeLineWidget::handleToolButtonNext_clicked);
    connect(m_app->videoController(),
            &QSpace::Core::Controllers::VideoController::changedEtalonLayer,
            this,
            &TimeLineWidget::handleChangeEtalonSnapshot);

    auto* popupView = ui->comboBox->view();

    connect(ui->comboBox, &QComboBox::activated, this, [this](int index) {
        if (index < 0)
            return;

        QUuid selectedLayerId = ui->comboBox->itemData(index).toUuid();
        emit  editLayerProperty(selectedLayerId);
    });
    // connect(ui->toolButtonEditLayerSettings,
    //         &QToolButton::clicked,
    //         this,
    //         &TimeLineWidget::handleToolButtonEditLayerSettings);
}

// =========== Кнопки смены кадров ===========
void TimeLineWidget::handleToolButtonPrev_clicked() {
    int currentValue = ui->TimeSlider->value();
    if (currentValue > ui->TimeSlider->minimum()) {
        ui->TimeSlider->setValue(currentValue - ui->TimeSlider->singleStep());
    }
}

void TimeLineWidget::handleToolButtonNext_clicked() {
    int currentValue = ui->TimeSlider->value();
    if (currentValue < ui->TimeSlider->maximum()) {
        ui->TimeSlider->setValue(currentValue + ui->TimeSlider->singleStep());
    }
}

// =========== Изменение значения слайдера (перетаскивание) ===========
void TimeLineWidget::handleTimeSliderChangeValue(int value) {
    setTextTimeLabelTextInternal();
    emit currentTimeStampValueChanged(value);
}

// =========== Изменение диапазона слайдера ===========
void TimeLineWidget::handleSnapshotsListChangeSize(int size) {
    if (size < 0) {
        ui->TimeSlider->setEnabled(false);
        ui->TimeSlider->setRange(0, 0);
        ui->TimeSlider->setValue(0);
        ui->label_Snapshot->setStyleSheet("QLabel {color: red;}");
        qCWarning(LogUI) << "unexpected size for slider: " << size;
        return;
    }

    ui->TimeSlider->setEnabled(true);
    ui->TimeSlider->setRange(1, size);
    ui->TimeSlider->setSingleStep(1);
    ui->TimeSlider->setPageStep(qMax(1, size / 10));
    ui->TimeSlider->setValue(1);
    setTextTimeLabelTextInternal();
}
void TimeLineWidget::handleToolButtonEditLayerSettings() {
}
// =========== Внутренние методы ===========
void TimeLineWidget::setTextTimeLabelTextInternal() {
    // ЗАЩИТА ОТ ВЫЛЕТОВ (Обязательно!)
    if (!m_app || !m_app->importController())
        return;

    int    value     = ui->TimeSlider->value();
    double timestamp = m_app->videoController()->getSnapshotTimeByIndex(value - 1);

    QString snapshotString = QString("Снимок: %1/%2 (T=%3 миллионов лет)")
                                 .arg(value)
                                 .arg(ui->TimeSlider->maximum())
                                 .arg(timestamp);
    ui->label_Snapshot->setText(snapshotString);
}
void TimeLineWidget::handleChangeEtalonSnapshot(const QList<QUuid>& list) {
    layerList = list;
    ui->comboBox->clear();
    for (const QUuid& id : layerList) {
        // Получаем красивое имя слоя из вашего менеджера по его UUID
        auto layer = m_app->layerController()->getLayerById(id);
        if (!layer)
            return;
        QString layerName = layer->name();
        // Передаем имя и прикрепляем к этому пункту скрытый UUID
        ui->comboBox->addItem(layerName, id);
    }
}
TimeLineWidget::~TimeLineWidget() {
    delete ui;
}

} // namespace QSpace::UI