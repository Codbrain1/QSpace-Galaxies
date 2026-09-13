
#include "AbstractView3D.h"
#include <qhashfunctions.h>
#include <qmetaobject.h>
#include <qobjectdefs.h>
#include "Session/Reflection.h"
#include "View3DSettings.h"
#include <cmath>

namespace QSpace::Visualize::Views {

QVariantMap AbstractView3D::getSettingsToVariantMap() const {
    if (!m_settings)
        return QVariantMap();

    QVariantMap settingsMap;
    auto        grid = m_settings->grid();

    // сериализуем сетку
    if (grid) {
        QVariantMap gridSettingsMap = Reflection::QObjectToVariantMap(grid);
        settingsMap.insert(QStringLiteral("grid"), gridSettingsMap);
    }

    // сериализуем оси
    auto axis = m_settings->axis();
    if (axis) {
        QVariantMap axisSettingsMap = Reflection::QObjectToVariantMap(axis);
        settingsMap.insert(QStringLiteral("axis"), axisSettingsMap);
    }

    // сериализуем настройки порта окн
    auto viewport = m_settings->viewport();
    if (viewport) {
        QVariantMap viewportSettings = Reflection::QObjectToVariantMap(viewport);
        settingsMap.insert(QStringLiteral("viewport"), viewportSettings);
    }
    return settingsMap;
}

bool AbstractView3D::setSettingsFromVariantMap(const QVariantMap& settings) {
    return false;
}

} // namespace QSpace::Visualize::Views