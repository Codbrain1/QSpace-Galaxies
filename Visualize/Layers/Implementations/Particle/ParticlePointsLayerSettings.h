#pragma once
#include <qtmetamacros.h>
#include "../../LayerSettings.h"

namespace QSpace::Visualize::Layers {
// ============================================================
// Настройки для ParticleRendererLayer — обычные точки-частицы
// ============================================================
class LAYERS_EXPORT ParticlePointsLayerSettings : public LayerSettings {
    Q_OBJECT
    // clang-format off
    Q_PROPERTY(float pointSizePx                 READ pointSizePx                 WRITE setPointSizePx                 NOTIFY changed)
    Q_PROPERTY(bool  autoSizePoint               READ autoSizePoint               WRITE setAutoSizePoint               NOTIFY changed)
    Q_PROPERTY(float minPointSizePx              READ minPointSizePx              WRITE setMinPointSizePx              NOTIFY changed)
    Q_PROPERTY(float maxPointSizePx              READ maxPointSizePx              WRITE setMaxPointSizePx              NOTIFY changed)
    Q_PROPERTY(bool  shadeAsSphere               READ shadeAsSphere               WRITE setShadeAsSphere               NOTIFY changed)
    Q_PROPERTY(bool  showOutRangeParticles       READ showOutRangeParticles       WRITE setShowOutRangeParticles       NOTIFY changed)

public:
    explicit ParticlePointsLayerSettings(QObject* parent = nullptr) : LayerSettings(parent) {}

    float pointSizePx() const { return m_pointSizePx; }
    void setPointSizePx(float s) { if (!qFuzzyCompare(m_pointSizePx, s)) { m_pointSizePx = s; emit changed(); } }

    // если true — размер точки масштабируется по значению colorByField (min..max диапазон)
    bool autoSizePoint() const { return m_autoSizePoint; }
    void setAutoSizePoint(bool v) { if (m_autoSizePoint != v) { m_autoSizePoint = v; emit changed(); } }

    float minPointSizePx() const { return m_minPointSizePx; }
    void setMinPointSizePx(float s) { if (!qFuzzyCompare(m_minPointSizePx, s)) { m_minPointSizePx = s; emit changed(); } }

    float maxPointSizePx() const { return m_maxPointSizePx; }
    void setMaxPointSizePx(float s) { if (!qFuzzyCompare(m_maxPointSizePx, s)) { m_maxPointSizePx = s; emit changed(); } }

    // круглый спрайт с псевдо-затенением (sqrt(1-r^2)) вместо плоского квадрата
    bool shadeAsSphere() const { return m_shadeAsSphere; }
    void setShadeAsSphere(bool v) { if (m_shadeAsSphere != v) { m_shadeAsSphere = v; emit changed(); } }
  
    bool showOutRangeParticles() { return m_showOutRangeParticles; }
    void setShowOutRangeParticles(bool s) { if ( m_showOutRangeParticles != s) { m_showOutRangeParticles = s; emit changed(); }}

    // clang-format on

    // Внутри ParticlePointsLayerSettings
    QString propertyDisplayName(const QString& propName) const override {
        static const QMap<QString, QString> particleNames = {
            {"pointSizePx", "Размер точки (px)"},
            {"sizeByField", "Масштаб от поля"},
            {"minPointSizePx", "Мин. размер точки"},
            {"maxPointSizePx", "Макс. размер точки"},
            {"shadeAsSphere", "Сферическое затенение"},
            {"showOutRangeParticles", "Отображать частицы вне диапазона"}};
        if (particleNames.contains(propName))
            return particleNames.value(propName);
        return LayerSettings::propertyDisplayName(propName);
    }

    bool isPropertyEnabled(const QString& propName) const override {
        if (propName == "pointSizePx") {
            return !autoSizePoint();
        } else if (propName == "minPointSizePx") {
            return autoSizePoint();
        } else if (propName == "maxPointSizePx") {
            return autoSizePoint();
        }
        return LayerSettings::isPropertyEnabled(propName); // По умолчанию всё доступно
    }

  private:
    bool m_showOutRangeParticles = false;

    bool  m_autoSizePoint  = false;
    float m_pointSizePx    = 4.0f;
    float m_minPointSizePx = 1.0f;
    float m_maxPointSizePx = 12.0f;

    bool m_shadeAsSphere = true;
};

} // namespace QSpace::Visualize::Layers