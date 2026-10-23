#pragma once

#include "Common/Structures/ColormapPresets.h"
#include "qcustomplot.h"
#include <QColor>
#include <QString>
#include <QUuid>
#include <QVector>
#include <QWidget>

namespace QSpace {
namespace Visualize {
namespace Layers {
class LayerSettings;
}
} // namespace Visualize
} // namespace QSpace

class ColorBarWidget : public QWidget {
    Q_OBJECT

  public:
    explicit ColorBarWidget(QWidget* parent = nullptr);
    ~ColorBarWidget() override;

    // Задать цветовую палитру (градиент) для отображения
    void setColorMap(const QSpace::Visualize::ColorMap& colorMap);

    // Применить настройки слоя (диапазон значений, палитра, видимость и т.д.)
    void setSettings(QSpace::Visualize::Layers::LayerSettings* settings);

    // Переключение режима: true — вертикальный, false — горизонтальный
    void setOrientation(bool isVertical);

    // Доступ к нижележащему QCustomPlot
    QCustomPlot* plot() const {
        return m_plot;
    }

  private:
    void setupPlot();
    void styleActiveAxis();
    void applyGradient(const QSpace::Visualize::ColorMap& colorMap);

  private slots:
    void handleUpdateValues();

  private:
    QCustomPlot*   m_plot       = nullptr;
    QCPColorScale* m_colorScale = nullptr;
    bool           m_isVertical = false;

    QSpace::Visualize::ColorMap               m_currentColorMap;
    QSpace::Visualize::Layers::LayerSettings* m_settings = nullptr;
};