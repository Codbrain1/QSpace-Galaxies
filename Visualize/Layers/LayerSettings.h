#pragma once
#include "Common/Structures/ColormapPresets.h"
#include <QColor>
#include <QMetaProperty>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVariantMap>

namespace QSpace::Visualize::Layers {

class LayerSettings : public QObject {
    Q_OBJECT
  public:
    enum class BlendMode { Alpha = 0, Additive = 1 };
    Q_ENUM(BlendMode)

    struct NumericConstraints {
        double min      = -1000000.0;
        double max      = 1000000.0;
        double step     = 1.0;
        int    decimals = 2;
    };

  private:
    // clang-format off
    Q_PROPERTY(bool          isVisible     READ isVisible     WRITE setVisible       NOTIFY changed)
    Q_PROPERTY(double        opacity       READ opacity       WRITE setOpacity       NOTIFY changed)
    Q_PROPERTY(QString       colorByField  READ colorByField  WRITE setColorByField  NOTIFY changed)
    Q_PROPERTY(QUuid         colorMapId    READ colorMapId    WRITE setColorMapId    NOTIFY changed)
    Q_PROPERTY(bool          useLogScale   READ useLogScale   WRITE setUseLogScale   NOTIFY changed)
    Q_PROPERTY(double        rangeMin      READ rangeMin      WRITE setRangeMin      NOTIFY changed)
    Q_PROPERTY(double        rangeMax      READ rangeMax      WRITE setRangeMax      NOTIFY changed)
    Q_PROPERTY(double        baseRangeMin  READ baseRangeMin  WRITE setBaseRangeMin  NOTIFY changed)
    Q_PROPERTY(double        baseRangeMax  READ baseRangeMax  WRITE setBaseRangeMax  NOTIFY changed)
    Q_PROPERTY(bool          autoRange     READ autoRange     WRITE setAutoRange     NOTIFY changed)
    Q_PROPERTY(int           zOrder        READ zOrder        WRITE setZOrder        NOTIFY changed)
    Q_PROPERTY(BlendMode     blendMode     READ blendMode     WRITE setBlendMode     NOTIFY changed)
    // clang-format on
  public:
    explicit LayerSettings(QObject* parent = nullptr) : QObject(parent) {
        m_colorMapId = Visualize::ColorMapPresets::getPresetByName("Plasma").id;
    }

    virtual ~LayerSettings() = default;

    bool isVisible() const {
        return m_isVisible;
    }

    void setVisible(bool v) {
        if (v != m_isVisible) {
            m_isVisible = v;
            emit changed();
        }
    }

    double opacity() const {
        return m_opacity;
    }

    void setOpacity(double o) {
        if (!qFuzzyCompare(o, m_opacity)) {
            m_opacity = o;
            emit changed();
        }
    }

    QString colorByField() const {
        return m_colorByField;
    }

    void setColorByField(const QString& f) {
        if (f != m_colorByField) {
            m_colorByField = f;
            emit changed();
        }
    }

    QUuid colorMapId() const {
        return m_colorMapId;
    }

    void setColorMapId(const QUuid& id) {
        if (id != m_colorMapId) {
            m_colorMapId = id;
            emit changed();
        }
    }

    bool useLogScale() const {
        return m_useLogScale;
    }

    void setUseLogScale(bool u) {
        if (u != m_useLogScale) {
            m_useLogScale = u;
            emit changed();
        }
    }

    double rangeMin() const {
        return m_rangeMin;
    }

    void setRangeMin(double v) {
        if (!qFuzzyCompare(v, m_rangeMin)) {
            m_rangeMin = v;
            emit changed();
        }
    }

    double rangeMax() const {
        return m_rangeMax;
    }

    void setRangeMax(double v) {
        if (!qFuzzyCompare(v, m_rangeMax)) {
            m_rangeMax = v;
            emit changed();
        }
    }

    double baseRangeMin() const {
        return m_baseRangeMin;
    }

    void setBaseRangeMin(double v) {
        if (!qFuzzyCompare(v, m_baseRangeMin)) {
            m_baseRangeMin = v;
            emit changed();
        }
    }

    double baseRangeMax() const {
        return m_baseRangeMax;
    }

    void setBaseRangeMax(double v) {
        if (!qFuzzyCompare(v, m_baseRangeMax)) {
            m_baseRangeMax = v;
            emit changed();
        }
    }

    bool autoRange() const {
        return m_autoRange;
    }

    void setAutoRange(bool a) {
        if (a != m_autoRange) {
            m_autoRange = a;
            emit changed();
        }
    }

    int zOrder() const {
        return m_zOrder;
    }

    void setZOrder(int z) {
        if (m_zOrder != z) {
            m_zOrder = z;
            emit changed();
        }
    }

    BlendMode blendMode() const {
        return m_blendMode;
    }

    void setBlendMode(BlendMode m) {
        if (m_blendMode != m) {
            m_blendMode = m;
            emit changed();
        }
    }

    QVariantMap toVariantMap() const {
        QVariantMap        map;
        const QMetaObject* mo = metaObject();
        for (int i = QObject::staticMetaObject.propertyCount(); i < mo->propertyCount(); ++i) {
            QMetaProperty prop = mo->property(i);
            map[prop.name()]   = prop.read(this);
        }
        return map;
    }

    void fromVariantMap(const QVariantMap& map) {
        const QMetaObject* mo = metaObject();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            int idx = mo->indexOfProperty(it.key().toUtf8().constData());
            if (idx >= 0)
                mo->property(idx).write(this, it.value());
        }
    }

    virtual QString propertyDisplayName(const QString& propName) const {
        static const QMap<QString, QString> baseNames = {{"isVisible", "Видимость слоя"},
                                                         {"opacity", "Непрозрачность частиц"},
                                                         {"colorByField", "Окрашивать по полю"},
                                                         {"colorMapId", "Цветовая карта"},
                                                         {"useLogScale", "Логарифмическая шкала"},
                                                         {"rangeMin", "Мин. значение диапазона"},
                                                         {"rangeMax", "Макс. значение диапазона"},
                                                         {"baseRangeMin", "Глобальный минимум"},
                                                         {"baseRangeMax", "Глобальный максимум"},
                                                         {"autoRange", "Авто-диапазон"},
                                                         {"zOrder", "Порядок отображения (Z)"},
                                                         {"blendMode", "Режим смешивания"}};
        //  {"pickable", "Доступен для выбора"}};
        return baseNames.value(propName, propName); // Если не нашли, вернем английское имя
    }

    virtual QString enumValueDisplayName(const QString& propName, const QString& enumKey) const {
        if (propName == "blendMode") {
            if (enumKey == "Alpha")
                return "Альфа-смешивание";
            if (enumKey == "Additive")
                return "Аддитивное (Добавление)";
        }
        return enumKey;
    }

    virtual NumericConstraints propertyConstraints(const QString& propName) const {
        NumericConstraints c;
        if (propName == "opacity") {
            c.min      = 0.0;
            c.max      = 1.0;
            c.step     = 0.05;
            c.decimals = 4;
        } else if (propName == "rangeMin" || propName == "rangeMax" || propName == "baseRangeMin" ||
                   propName == "baseRangeMax") {
            // ДИНАМИЧЕСКИЙ РАСЧЕТ: шаг — это 1% от текущего глобального диапазона
            double delta = m_baseRangeMax - m_baseRangeMin;
            if (delta <= 0.0)
                delta = 1.0;

            c.step     = delta / 100.0;
            c.decimals = 8;

            c.min = m_baseRangeMin - delta * 10; // Позволяем крутить чуть шире диапазона
            c.max = m_baseRangeMax + delta * 10;
        }
        return c;
    }

    virtual bool isPropertyEnabled(const QString& propName) const {
        if (propName == "rangeMin") {
            return !autoRange();
        } else if (propName == "rangeMax") {
            return !autoRange();
        } else if (propName == "baseRangeMin") {
            return false;
        } else if (propName == "baseRangeMax") {
            return false;
        }
        return true; // По умолчанию всё доступно
    }

  signals:
    void changed();

  private:
    bool      m_isVisible = true; // видимость слоя
    double    m_opacity   = 1.0;  // непрозрачномсть частиц
    QString   m_colorByField;     // имя поля, по которому окрашиваются частицы
    QUuid     m_colorMapId; // идентификатор цветовой карты, используемой для окрашивания частиц
    bool      m_useLogScale  = true; // использовать ли логарифмическую шкалу для окрашивания частиц
    double    m_rangeMin     = 0.0;  // минимальное значение диапазона окрашивания частиц
    double    m_rangeMax     = 100.0; // максимальное значение диапазона окрашивания частиц
    double    m_baseRangeMin = 0.0;   // глобальное минимальное значение
    double    m_baseRangeMax = 100.0; // глобальное максимальное значение
    bool      m_autoRange    = true;  // автоматически ли подбирать диапазон окрашивания частиц
    int       m_zOrder       = 0;
    BlendMode m_blendMode    = BlendMode::Alpha;
};

} // namespace QSpace::Visualize::Layers