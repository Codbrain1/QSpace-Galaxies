#pragma once
#include <QColor>
#include "../../LayerSettings.h"

// ============================================================
// Настройки для BindingRendererLayer — связи/бонды между частицами
// (например SPH-соседи, орбитальные связи, molecular bonds — в зависимости от домена)
// ============================================================
namespace QSpace::Visualize::Layers {

class LAYERS_EXPORT BinningPointsLayerSettings : public LayerSettings {
    Q_OBJECT
    // clang-format off
    Q_PROPERTY(float  lineWidth    READ lineWidth    WRITE setLineWidth    NOTIFY changed)
    Q_PROPERTY(double cellSize     READ cellSize     WRITE setCellSize     NOTIFY changed)
    Q_PROPERTY(float  maxDistance  READ maxDistance  WRITE setMaxDistance  NOTIFY changed)
    Q_PROPERTY(bool   fadeByLength READ fadeByLength WRITE setFadeByLength NOTIFY changed)
    Q_PROPERTY(QColor lineColor    READ lineColor    WRITE setLineColor    NOTIFY changed)

public:
    explicit BinningPointsLayerSettings(QObject* parent = nullptr) : LayerSettings(parent) {}

    float lineWidth() const { return m_lineWidth; }
    void setLineWidth(float w) { if (!qFuzzyCompare(m_lineWidth, w)) { m_lineWidth = w; emit changed(); } }

    // размер ячейки пространственной сетки для поиска соседей/связей
    double cellSize() const { return m_cellSize; }
    void setCellSize(double c) { if (!qFuzzyCompare(m_cellSize, c)) { m_cellSize = c; emit changed(); } }

    // порог расстояния — связь рисуется только если частицы ближе этого значения
    float maxDistance() const { return m_maxDistance; }
    void setMaxDistance(float d) { if (!qFuzzyCompare(m_maxDistance, d)) { m_maxDistance = d; emit changed(); } }

    // прозрачность линии уменьшается с увеличением расстояния между частицами
    bool fadeByLength() const { return m_fadeByLength; }
    void setFadeByLength(bool v) { if (m_fadeByLength != v) { m_fadeByLength = v; emit changed(); } }

    QColor lineColor() const { return m_lineColor; }
    void setLineColor(const QColor& c) { if (m_lineColor != c) { m_lineColor = c; emit changed(); } }

    // clang-format on

  private:
    float  m_lineWidth    = 1.0f;
    double m_cellSize     = 5.0;
    float  m_maxDistance  = 1.0f;
    bool   m_fadeByLength = true;
    QColor m_lineColor    = QColor(200, 200, 200, 120);
};
} // namespace QSpace::Visualize::Layers