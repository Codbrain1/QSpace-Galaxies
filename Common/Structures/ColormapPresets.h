#pragma once
#include <QList>
#include <QString>
#include <QUuid>
#include <QVector>
#include <qcontainerfwd.h>
#include <qlist.h>

namespace QSpace::Visualize {

struct ColorPoint {
    double x;       // Позиция на шкале (0.0 - 1.0)
    double r, g, b; // Цвет (0.0 - 1.0)
    bool   operator==(const ColorPoint&) const = default;
};

struct ColorMap {
    QUuid               id;
    QString             name;
    QString             filePath;
    QVector<ColorPoint> points;
    bool                isPreset = false;

    bool operator==(const ColorMap&) const = default;
};

class ColorMapPresets {
  public:
    static QList<ColorMap> getStandardPresets() {
        QList<ColorMap> presets;

        // Лямбда теперь принимает фиксированный UUID в виде строки
        auto addPreset =
            [&](const QString& idStr, const QString& name, const QVector<ColorPoint>& pts) {
                ColorMap map;
                map.id       = QUuid::fromString(idStr); // ФИКСИРОВАННЫЙ ID
                map.name     = name;
                map.isPreset = true;
                map.points   = pts;
                presets.append(map);
            };

        // Сгенерированные один раз статичные UUID для пресетов
        addPreset("{1b4d0001-0000-0000-0000-000000000001}",
                  "Viridis",
                  {{0.00, 0.267, 0.004, 0.329},
                   {0.25, 0.230, 0.322, 0.545},
                   {0.50, 0.127, 0.566, 0.550},
                   {0.75, 0.369, 0.788, 0.382},
                   {1.00, 0.993, 0.906, 0.143}});
        addPreset("{1b4d0002-0000-0000-0000-000000000002}",
                  "Inferno",
                  {{0.00, 0.001, 0.000, 0.004},
                   {0.25, 0.330, 0.007, 0.370},
                   {0.50, 0.730, 0.210, 0.230},
                   {0.75, 0.980, 0.640, 0.160},
                   {1.00, 0.980, 0.990, 0.690}});
        addPreset("{1b4d0003-0000-0000-0000-000000000003}",
                  "Plasma",
                  {{0.00, 0.050, 0.020, 0.520},
                   {0.25, 0.410, 0.040, 0.650},
                   {0.50, 0.740, 0.200, 0.540},
                   {0.75, 0.950, 0.510, 0.260},
                   {1.00, 0.940, 0.940, 0.130}});
        addPreset("{1b4d0004-0000-0000-0000-000000000004}",
                  "Magma",
                  {{0.00, 0.000, 0.000, 0.010},
                   {0.25, 0.140, 0.040, 0.310},
                   {0.50, 0.440, 0.080, 0.490},
                   {0.75, 0.800, 0.220, 0.380},
                   {1.00, 0.980, 0.920, 0.700}});
        addPreset("{1b4d0005-0000-0000-0000-000000000005}",
                  "CoolToWarm",
                  {{0.00, 0.230, 0.290, 0.750},
                   {0.50, 0.860, 0.860, 0.860},
                   {1.00, 0.700, 0.010, 0.140}});
        addPreset("{1b4d0006-0000-0000-0000-000000000006}",
                  "Rainbow",
                  {{0.00, 0.0, 0.0, 1.0},
                   {0.25, 0.0, 1.0, 1.0},
                   {0.50, 0.0, 1.0, 0.0},
                   {0.75, 1.0, 1.0, 0.0},
                   {1.00, 1.0, 0.0, 0.0}});
        addPreset("{1b4d0007-0000-0000-0000-000000000007}",
                  "Grayscale",
                  {{0.00, 0.0, 0.0, 0.0}, {1.00, 1.0, 1.0, 1.0}});
        addPreset("{1b4d0008-0000-0000-0000-000000000008}",
                  "bwr modify",
                  {{0.00, 0.000, 0.000, 0.050},   // Глубокий черный/синий на самом минимуме
                   {0.15, 0.000, 0.000, 0.850},   // Насыщенный синий
                   {0.35, 0.450, 0.000, 0.650},   // Переходный фиолетовый/пурпурный
                   {0.55, 0.900, 0.000, 0.000},   // Яркий красный
                   {0.75, 1.000, 0.450, 0.000},   // Оранжевый
                   {0.90, 1.000, 0.900, 0.000},   // Насыщенный желтый
                   {1.00, 1.000, 1.000, 1.000}}); // Белое горячее ядро в центре
        return presets;
    }

    static ColorMap getPresetByName(const QString& name) {
        for (const auto& preset : getStandardPresets()) {
            if (preset.name == name)
                return preset;
        }
        return getStandardPresets().first();
    }
};

} // namespace QSpace::Visualize