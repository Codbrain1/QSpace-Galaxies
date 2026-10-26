
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
    if (!m_settings)
        return false;

    bool applied = false;
    if (const auto grid = m_settings->grid(); settings.contains(QStringLiteral("grid")) && grid) {
        const auto result = Reflection::variantMapToQObject(settings.value(QStringLiteral("grid")).toMap(), grid);
        applied |= result.success;
    }
    if (const auto axis = m_settings->axis(); settings.contains(QStringLiteral("axis")) && axis) {
        const auto result = Reflection::variantMapToQObject(settings.value(QStringLiteral("axis")).toMap(), axis);
        applied |= result.success;
    }
    if (const auto viewport = m_settings->viewport(); settings.contains(QStringLiteral("viewport")) && viewport) {
        const auto result =
            Reflection::variantMapToQObject(settings.value(QStringLiteral("viewport")).toMap(), viewport);
        applied |= result.success;
    }
    return applied;
}

} // namespace QSpace::Visualize::Views