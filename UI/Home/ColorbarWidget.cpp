#include "ColorbarWidget.h"
#include "Physics/DimensionConverter/PhysicalUnits.h"
#include "Visualize/Layers/LayerSettings.h"
#include <QVBoxLayout>
#include <algorithm>
#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qcustomplot.h>

ColorBarWidget::ColorBarWidget(QWidget* parent) : QWidget(parent) {
    setupPlot();
}

ColorBarWidget::~ColorBarWidget() = default;

void ColorBarWidget::setupPlot() {
    QVBoxLayout* widgetLayout = new QVBoxLayout(this);
    widgetLayout->setContentsMargins(0, 0, 0, 0);

    m_plot = new QCustomPlot(this);
    widgetLayout->addWidget(m_plot);

    QColor themeBgColor = this->palette().color(QPalette::Window);
    m_plot->setBackground(QBrush(themeBgColor));

    // Инициализируем шкалу в горизонтальном режиме по умолчанию
    setOrientation(false);
}

void ColorBarWidget::setOrientation(bool isVertical) {
    if (!m_plot)
        return;

    m_isVertical = isVertical;

    // 1. Полностью очищаем layout графика.
    // Это удаляет старый экземпляр m_colorScale вместе с закэшированным горизонтальным QImage
    m_plot->plotLayout()->clear();

    // 2. Создаем новый чистый экземпляр QCPColorScale
    m_colorScale = new QCPColorScale(m_plot);

    // 3. ВАЖНО: Задаем тип ориентации ДО добавления и ДО установки градиента
    if (m_isVertical) {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_colorScale->setType(QCPAxis::atRight);
        m_plot->plotLayout()->setMargins(QMargins(10, 15, 65, 15));
    } else {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_colorScale->setType(QCPAxis::atBottom);
        m_plot->plotLayout()->setMargins(QMargins(15, 10, 15, 45));
    }

    m_colorScale->setDataRange(QCPRange(0.0, 100.0));
    m_plot->plotLayout()->addElement(0, 0, m_colorScale);

    // 4. Применяем цвета темы и шрифты к активной оси
    styleActiveAxis();

    // 5. Применяем градиент (теперь он сгенерирует QImage строго под текущий mType)
    applyGradient(m_currentColorMap);

    // 6. Обновляем значения и диапазоны из m_settings (если они уже заданы)
    handleUpdateValues();

    // 7. Принудительно обновляем геометрию layout и перерисовываем
    m_plot->plotLayout()->update(QCPLayoutElement::upLayout);
    m_plot->replot();
}

void ColorBarWidget::styleActiveAxis() {
    if (!m_colorScale || !m_colorScale->axis())
        return;

    QColor themeTextColor = this->palette().color(QPalette::WindowText);
    QPen   axisPen(themeTextColor);

    QCPAxis* activeAxis = m_colorScale->axis();
    activeAxis->setRangeReversed(false);
    activeAxis->setLabelColor(themeTextColor);
    activeAxis->setTickLabelColor(themeTextColor);
    activeAxis->setBasePen(axisPen);
    activeAxis->setTickPen(axisPen);
    activeAxis->setSubTickPen(axisPen);
    activeAxis->setTickLabelSide(QCPAxis::lsOutside);

    QFont labelFont = activeAxis->labelFont();
    labelFont.setPointSize(10);
    activeAxis->setLabelFont(labelFont);

    QFont tickFont = activeAxis->tickLabelFont();
    tickFont.setPointSize(9);
    activeAxis->setTickLabelFont(tickFont);
}

void ColorBarWidget::applyGradient(const QSpace::Visualize::ColorMap& colorMap) {
    if (!m_colorScale || !m_plot)
        return;

    QCPColorGradient gradient;
    gradient.clearColorStops();

    if (colorMap.points.isEmpty()) {
        gradient = QCPColorGradient::gpJet;
    } else {
        QVector<QSpace::Visualize::ColorPoint> sortedPoints = colorMap.points;
        std::sort(sortedPoints.begin(),
                  sortedPoints.end(),
                  [](const QSpace::Visualize::ColorPoint& a, const QSpace::Visualize::ColorPoint& b) {
                      return a.x < b.x;
                  });

        for (const QSpace::Visualize::ColorPoint& point : sortedPoints) {
            const double clampedPos = qBound(0.0, point.x, 1.0);
            gradient.setColorStopAt(clampedPos, QColor::fromRgbF(point.r, point.g, point.b));
        }
    }

    // Принудительный сброс на случай повторного вызова с тем же градиентом
    m_colorScale->setGradient(gradient.inverted());
    m_colorScale->setGradient(gradient);

    m_currentColorMap = colorMap;
    m_plot->replot();
}

void ColorBarWidget::setColorMap(const QSpace::Visualize::ColorMap& colorMap) {
    applyGradient(colorMap);
}

void ColorBarWidget::setSettings(QSpace::Visualize::Layers::LayerSettings* settings) {
    if (!settings)
        return;

    if (m_settings == settings)
        return;

    if (m_settings)
        disconnect(m_settings, nullptr, this, nullptr);

    m_settings = settings;

    connect(m_settings,
            &QSpace::Visualize::Layers::LayerSettings::changed,
            this,
            &ColorBarWidget::handleUpdateValues);
    connect(m_settings,
            &QSpace::Visualize::Layers::LayerSettings::calculatedRangeChanged,
            this,
            &ColorBarWidget::handleUpdateValues);

    connect(m_settings, &QObject::destroyed, this, [this]() { m_settings = nullptr; });

    handleUpdateValues();
}

void ColorBarWidget::handleUpdateValues() {
    if (!m_settings || !m_colorScale || !m_plot)
        return;

    QString colorByfield = m_settings->colorByField();
    QString dimension;
    if (colorByfield == "Mass") {
        dimension = "Msun/pc^2";
    } else if (colorByfield == "Density") {
        dimension = "Msun/pc^3";
    } else if (colorByfield == "Energy") {
        dimension = m_settings->hasPhysicalGamma() ? "K" : QString();
    } else if (colorByfield == "Velocity") {
        dimension = "km/s";
    }

    m_colorScale->axis()->setLabel(colorByfield + "(" + dimension + ")");

    QCPRange range;
    double   rawMin = 0;
    double   rawMax = 0;
    if (m_settings->autoRange()) {
        rawMin = m_settings->baseRangeMin();
        rawMax = m_settings->baseRangeMax();
    } else {
        rawMin = m_settings->rangeMin();
        rawMax = m_settings->rangeMax();
    }

    auto pu = QSpace::Physics::PhysicalUnits::fromSimParams(m_settings->physicalKm(),
                                                            m_settings->physicalKr(),
                                                            m_settings->physicalGamma(),
                                                            m_settings->hasPhysicalGamma());
    double scaleFactor = QSpace::Physics::fieldPhysicalScale(pu, m_settings->colorByField());
    rawMin *= scaleFactor;
    rawMax *= scaleFactor;

    if (m_settings->useLogScale()) {
        // ВАЖНО: меняем тип шкалы через сам QCPColorScale, чтобы градиент тоже знал про логарифм
        m_colorScale->setDataScaleType(QCPAxis::stLogarithmic);

        QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
        logTicker->setLogBase(10.0);
        logTicker->setTickCount(6);

        m_colorScale->axis()->setTicker(logTicker);
        m_colorScale->axis()->setNumberFormat("eb");
        m_colorScale->axis()->setNumberPrecision(0);

        double safeMin = (rawMin > 1e-12 * std::max(std::abs(rawMax), 1.0)) ? rawMin : rawMax * 1e-4;
        safeMin        = std::max(safeMin, 1e-12);
        double safeMax = std::max(rawMax, safeMin * 10.0);

        range = QCPRange(safeMin, safeMax);
    } else {
        m_colorScale->setDataScaleType(QCPAxis::stLinear);
        m_colorScale->axis()->setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker));

        m_colorScale->axis()->setNumberFormat("gb");
        m_colorScale->axis()->setNumberPrecision(3);
        range = QCPRange(rawMin, rawMax);
    }

    // Устанавливаем диапазон данных и синхронизируем ось
    m_colorScale->setDataRange(range);
    m_colorScale->axis()->setRange(range);

    m_plot->replot();
}