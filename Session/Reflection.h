#pragma once
#include <QAssociativeIterable>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMetaProperty>
#include <QSequentialIterable>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <qcontainerfwd.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qvariant.h>
#include <Common/Structures/FileSchemeStructures.h>
#include <concepts>

namespace QSpace::Reflection {

struct DeserializationResult {
    bool        success            = false; // Удалось ли записать хотя бы одно свойство
    int         setPropertiesCount = 0;     // Сколько свойств успешно записано
    int         totalKeysInMap     = 0;     // Сколько всего ключей было в карте
    QStringList unknownKeys;                // Ключи из карты, которых нет в QObject
    QStringList readOnlyKeys; // Ключи, которые есть, но они только для чтения (isWritable == false)
    QStringList
        writeFailedKeys; // Ключи, для которых prop.write() вернул false (ошибка типа/валидации)

    // Проверка: все ли ключи из QVariantMap удалось применить
    bool isFullyApplied() const {
        return success && unknownKeys.isEmpty() && readOnlyKeys.isEmpty() &&
               writeFailedKeys.isEmpty();
    }
};

namespace details {
// --- сериализация гаджетов ---
QVariant    serializeValue(const QVariant& value, const QMetaProperty& prop = QMetaProperty());
QVariantMap serializeGadget(const QVariant& gadgetValue);

QVariant deserializeValue(int                  targetTypeId,
                          const QVariant&      flatValue,
                          const QMetaProperty& prop = QMetaProperty());
QVariant deserializeGadget(int targetTypeId, const QVariantMap& map);

// Пытается найти QMetaEnum для произвольного enum-QMetaType,
// не имея под рукой QMetaProperty (например, для элементов списка)
QMetaEnum enumeratorForMetaType(const QMetaType& mt);

// --- сериализация QObject ---
QVariantMap           QObjectToVariantMap(const QObject* obj);
DeserializationResult variantMapToQObject(const QVariantMap& map, QObject* obj);

} // namespace details

// ------------------------------------------------------------------
// Публичное API для случая, когда статический тип T известен на месте вызова
// ------------------------------------------------------------------

template <typename T>
    requires std::derived_from<T, QObject>
inline QVariantMap QObjectToVariantMap(const T* obj) {
    return details::QObjectToVariantMap(obj);
}

template <typename T>
    requires std::derived_from<T, QObject>
inline DeserializationResult variantMapToQObject(const QVariantMap& map, const T* obj) {
    return details::variantMapToQObject(map, obj);
}

// преобразует шаблонный тип T в QVariantMap
template <typename T> QVariantMap gadgetToVariantMap(const T& obj) {
    QVariantMap        map;
    const QMetaObject& mo = T::staticMetaObject;
    for (int i = 0; i < mo.propertyCount(); ++i) {
        QMetaProperty prop = mo.property(i);
        if (!prop.isReadable())
            continue;
        map.insert(QString::fromLatin1(prop.name()),
                   details::serializeValue(prop.readOnGadget(&obj), prop));
    }
    return map;
}

// преобразует QVariantMap в тип T
template <typename T> T variantMapToGadget(const QVariantMap& map) {
    T                  obj{};
    const QMetaObject& mo = T::staticMetaObject;
    for (int i = 0; i < mo.propertyCount(); ++i) {
        QMetaProperty prop = mo.property(i);
        if (!prop.isWritable())
            continue;
        auto it = map.find(QString::fromLatin1(prop.name()));
        if (it == map.end())
            continue;
        QVariant restored = details::deserializeValue(prop.userType(), it.value());
        prop.writeOnGadget(&obj, restored);
    }
    return obj;
}

// ------------------------------------------------------------------
// Публичноое API для сериализации схемы файла
// ------------------------------------------------------------------

// сериализация и восстановление схемы отдельно потому что она обернута в вариант
QVariantMap            readSchemeToVariant(const QSpace::IO::ReadScheme& scheme);
QSpace::IO::ReadScheme variantToReadScheme(const QVariantMap& map);
} // namespace QSpace::Reflection