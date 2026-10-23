#pragma once
#include <QColor>
#include <QObject>
#include <qobject.h>
#include <qtmetamacros.h>

namespace QSpace::Visualize::Views::View3D {

// ============================================================
// Сетка на плоскости
// ============================================================
class GridSettings : public QObject {
    Q_OBJECT
    // clang-format off

    Q_PROPERTY(bool   visible READ visible WRITE setVisible NOTIFY changed)
    Q_PROPERTY(QColor color   READ color   WRITE setColor   NOTIFY changed)
    Q_PROPERTY(float  extent  READ extent  WRITE setExtent  NOTIFY changed) 
    Q_PROPERTY(int    lineCount READ lineCount WRITE setLineCount NOTIFY changed)
  public:
    explicit GridSettings(QObject* parent = nullptr) : QObject(parent) {}

    bool visible() const { return m_visible; }
    void setVisible(bool v) { if (m_visible != v) { m_visible = v; emit changed(); } }

    QColor color() const { return m_color; }
    void setColor(const QColor& c) { if (m_color != c) { m_color = c; emit changed(); } }


    float extent() const { return m_extent; }
    void setExtent(float e) { if (!qFuzzyCompare(m_extent, e)) { m_extent = e; emit changed(); } }

    int lineCount() const { return m_lineCount; }
    void setLineCount(int n) { if (m_lineCount != n) { m_lineCount = n; emit changed(); } }

    // clang-format on
  signals:
    void changed();

  private:
    bool   m_visible   = false;               // видимость сетки
    QColor m_color     = QColor(90, 90, 100); // цвет сетки
    float  m_extent    = 10.0f;               // половина от общего размера сетки
    int    m_lineCount = 16;                  // число линий на сетке
};

// ============================================================
// Координатные оси + подписи делений (числа рисуются через QPainter-оверлей)
// ============================================================
class AxisSettings : public QObject {
    Q_OBJECT
    // clang-format off
    Q_PROPERTY(bool   visible       READ visible       WRITE setVisible       NOTIFY changed)
    Q_PROPERTY(bool   showLabels    READ showLabels    WRITE setShowLabels    NOTIFY changed)
    Q_PROPERTY(bool   showTicks     READ showTicks     WRITE setShowTicks     NOTIFY changed)
    Q_PROPERTY(int    tickCount     READ tickCount     WRITE setTickCount     NOTIFY changed)
    Q_PROPERTY(QColor colorX        READ colorX        WRITE setColorX        NOTIFY changed)
    Q_PROPERTY(QColor colorY        READ colorY        WRITE setColorY        NOTIFY changed)
    Q_PROPERTY(QColor colorZ        READ colorZ        WRITE setColorZ        NOTIFY changed)
    Q_PROPERTY(QString labelX        READ labelX        WRITE setLabelX        NOTIFY changed)
    Q_PROPERTY(QString labelY        READ labelY        WRITE setLabelY        NOTIFY changed)
    Q_PROPERTY(QString labelZ        READ labelZ        WRITE setLabelZ        NOTIFY changed)
    Q_PROPERTY(QString unitLabel    READ unitLabel     WRITE setUnitLabel     NOTIFY changed)
    Q_PROPERTY(QString labelFontFamily READ labelFontFamily WRITE setLabelFontFamily NOTIFY changed)
    Q_PROPERTY(int     labelFontSize READ labelFontSize WRITE setLabelFontSize NOTIFY changed)
    
    public:
    explicit AxisSettings(QObject* parent = nullptr) : QObject(parent) {}

    bool visible() const { return m_visible; }
    void setVisible(bool v) { if (m_visible != v) { m_visible = v; emit changed(); } }

    bool showLabels() const { return m_showLabels; }
    void setShowLabels(bool v) { if (m_showLabels != v) { m_showLabels = v; emit changed(); } }

    bool showTicks() const { return m_showTicks; }
    void setShowTicks(bool v) { if (m_showTicks != v) { m_showTicks = v; emit changed(); } }
    
    int tickCount() const { return m_tickCount; }
    void setTickCount(int n) { if (m_tickCount != n) { m_tickCount = n; emit changed(); } }

    QColor colorX() const { return m_colorX; }
    void setColorX(const QColor& c) { if (m_colorX != c) { m_colorX = c; emit changed(); } }

    QColor colorY() const { return m_colorY; }
    void setColorY(const QColor& c) { if (m_colorY != c) { m_colorY = c; emit changed(); } }

    QColor colorZ() const { return m_colorZ; }
    void setColorZ(const QColor& c) { if (m_colorZ != c) { m_colorZ = c; emit changed(); } }
    
    QString unitLabel() const { return m_unitLabel; }
    void setUnitLabel(const QString& u) { if (m_unitLabel != u) { m_unitLabel = u; emit changed(); } }
    
    QString labelX() const {return m_labelX;}
    void setLabelX(const QString& label) { if(m_labelX != label) {m_labelX = label;emit changed();}}

    QString labelY() const {return m_labelY;}
    void setLabelY(const QString& label) { if(m_labelY != label) {m_labelY = label;emit changed();}}

    QString labelZ() const {return m_labelZ;}
    void setLabelZ(const QString& label) { if(m_labelZ != label) {m_labelZ = label;emit changed();}}

    QString labelFontFamily() const { return m_labelFontFamily; }
    void setLabelFontFamily(const QString& f) { if (m_labelFontFamily != f) { m_labelFontFamily = f; emit changed(); } }

    int labelFontSize() const { return m_labelFontSize; }
    void setLabelFontSize(int s) { if (m_labelFontSize != s) { m_labelFontSize = s; emit changed(); } }

    // clang-format on

  signals:
    void changed();

  private:
    bool    m_visible         = true;                 // видимость осей
    bool    m_showLabels      = true;                 // видимость меток
    bool    m_showTicks       = true;                 // видимость засечек
    int     m_tickCount       = 8;                    // число зачечек
    QColor  m_colorX          = QColor(220, 60, 60);  // цвет оси X
    QColor  m_colorY          = QColor(60, 200, 90);  // цвет оси Y
    QColor  m_colorZ          = QColor(60, 120, 220); // цвет оси Z
    QString m_labelX          = "X";                  // подпись оси X
    QString m_labelY          = "Y";                  // подпись оси Y
    QString m_labelZ          = "Z";                  // подпись оси Z
    QString m_unitLabel       = "kpc";                // подпись размерности
    QString m_labelFontFamily = "Segoe UI";           // тип шрифта
    int     m_labelFontSize   = 10;                   // размер шрифта
};

// ============================================================
// Настройки самого 3D-вьюпорта: камера, фон, проекция, гизмо
// ============================================================
class ViewportSettings : public QObject {
    Q_OBJECT
    // clang-format off
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY changed)
    Q_PROPERTY(bool   orthographic    READ orthographic    WRITE setOrthographic    NOTIFY changed)
    Q_PROPERTY(float  fovYDegrees     READ fovYDegrees     WRITE setFovYDegrees     NOTIFY changed)
    Q_PROPERTY(float  nearClip        READ nearClip        WRITE setNearClip        NOTIFY changed)
    Q_PROPERTY(float  farClip         READ farClip         WRITE setFarClip         NOTIFY changed)
    Q_PROPERTY(bool   gizmoVisible    READ gizmoVisible    WRITE setGizmoVisible    NOTIFY changed)
    Q_PROPERTY(int    multisamples    READ multisamples    WRITE setMultisamples    NOTIFY changed)
public:
    explicit ViewportSettings(QObject* parent = nullptr) : QObject(parent) {}

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& c) { if (m_backgroundColor != c) { m_backgroundColor = c; emit changed(); } }

    bool orthographic() const { return m_orthographic; }
    void setOrthographic(bool v) { if (m_orthographic != v) { m_orthographic = v; emit changed(); } }

    float fovYDegrees() const { return m_fovYDegrees; }
    void setFovYDegrees(float f) { if (!qFuzzyCompare(m_fovYDegrees, f)) { m_fovYDegrees = f; emit changed(); } }

    float nearClip() const { return m_nearClip; }
    void setNearClip(float n) { if (!qFuzzyCompare(m_nearClip, n)) { m_nearClip = n; emit changed(); } }

    float farClip() const { return m_farClip; }
    void setFarClip(float f) { if (!qFuzzyCompare(m_farClip, f)) { m_farClip = f; emit changed(); } }

    bool gizmoVisible() const { return m_gizmoVisible; }
    void setGizmoVisible(bool v) { if (m_gizmoVisible != v) { m_gizmoVisible = v; emit changed(); } }

    int multisamples() const { return m_multisamples; }
    void setMultisamples(int n) { if (m_multisamples != n) { m_multisamples = n; emit changed(); } }

    // clang-format on
  signals:
    void changed();

  private:
    // материалы для изучения
    // https://vuzdoc.ru/76377/tehnika/proektsii

    QColor m_backgroundColor = QColor(13, 13, 20); // фон отображаемого окна
    bool   m_orthographic    = true; // Переключатель между двумя типами проекции: Перспективная
                                     // проекция и Ортографическая (параллельная) проекция
    float m_fovYDegrees = 45.0f;     // по вертикали (угол обзора камеры), в градусах. Используется
    // только когда включена перспективная проекция

    // Границы отсечения по глубине (near/far clipping planes). Определяют диапазон
    // расстояний от камеры, в котором объекты вообще видны и участвуют в рендеринге:
    // Параметры left и right задают координаты левой и правой вертикальной плоскости отсечения.
    // Параметры bottom и top определяют координаты верхней и нижней плоскости отсечения. Параметры
    // zNear и zFar задают расстояние от точки (0, 0, 0) до ближней и дальней плоскостей отсечения и
    // могут быть положительными и отрицательными.
    float m_nearClip = -1000.0f; // стоит учесть что в перспективной проекции этот парметр должен
                                 // быть строго больше нуля
    float m_farClip = 1000.0f;

    // Показывать ли координатный гизмо — маленький 3D-компас (обычно в углу экрана)
    bool m_gizmoVisible = true;

    // MSAA (MultiSample Anti-Aliasing) — количество семплов на пиксель для
    // сглаживания краёв геометрии
    int m_multisamples = 4;
};

// ============================================================
// Верхнеуровневый агрегат — то, что хранит AbstractView3D
// ============================================================
class View3DSettings : public QObject {
    Q_OBJECT
    // clang-format off
    Q_PROPERTY(GridSettings*     grid     READ grid     CONSTANT)
    Q_PROPERTY(AxisSettings*     axis     READ axis     CONSTANT)
    Q_PROPERTY(ViewportSettings* viewport READ viewport CONSTANT)
    // clang-format on

  public:
    explicit View3DSettings(QObject* parent = nullptr)
        : QObject(parent),
          m_grid(new GridSettings(this)),
          m_axis(new AxisSettings(this)),
          m_viewport(new ViewportSettings(this)) {
        // прокидываем изменения дочерних настроек наверх одним сигналом —
        // удобно для View3D, чтобы подписаться один раз и перерисовываться на любое изменение
        connect(m_grid, &GridSettings::changed, this, &View3DSettings::anyChanged);
        connect(m_axis, &AxisSettings::changed, this, &View3DSettings::anyChanged);
        connect(m_viewport, &ViewportSettings::changed, this, &View3DSettings::anyChanged);
    }

    GridSettings* grid() const {
        return m_grid;
    }

    AxisSettings* axis() const {
        return m_axis;
    }

    ViewportSettings* viewport() const {
        return m_viewport;
    }

  signals:
    void anyChanged();

  private:
    GridSettings*     m_grid;
    AxisSettings*     m_axis;
    ViewportSettings* m_viewport;
};

// ============================================================
// Настройки самого 3D-вьюпорта: камера, фон, проекция, гизмо
// ============================================================

} // namespace QSpace::Visualize::Views::View3D