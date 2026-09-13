#include "Reflection.h"
#include "Common/Structures/FileSchemeStructures.h"
#include <QMetaAssociation>
#include <QMetaSequence>
#include <qassociativeiterable.h>
#include <qcontainerfwd.h>
#include <qiterable.h>
#include <qmetacontainer.h>
#include <qmetaobject.h>
#include <qobjectdefs.h>
#include <qsequentialiterable.h>
#include <qtpreprocessorsupport.h>
#include <qvariant.h>
#include <variant>

namespace QSpace::Reflection {
namespace details {

QMetaEnum enumeratorForMetaType(const QMetaType& mt) {
    const QMetaObject* mo = mt.metaObject();
    if (!mo)
        return QMetaEnum();

    // mt.name() обычно выглядит как "QSpace::Visualize::EntityType"
    QByteArray typeName = mt.name();
    int        sep      = typeName.lastIndexOf("::");
    QByteArray enumName = sep >= 0 ? typeName.mid(sep + 2) : typeName;

    int idx = mo->indexOfEnumerator(enumName.constData());
    return idx >= 0 ? mo->enumerator(idx) : QMetaEnum();
}

// конвертирует любой гаджет в вариант
QVariant serializeValue(const QVariant& value, const QMetaProperty& prop) {
    if (!value.isValid() || value.isNull())
        return QVariant();

    QMetaType mt(value.userType());

    if (mt.id() == QMetaType::QString || mt.id() == QMetaType::QByteArray) {
        return value;
    }

    const bool isEnumViaProp = prop.isValid() && prop.isEnumType();
    const bool isEnumViaType = mt.flags().testFlag(QMetaType::IsEnumeration);

    // преобразуем перечисление в строку
    if (isEnumViaProp || isEnumViaType) {
        QMetaEnum metaEnum = isEnumViaProp ? prop.enumerator() : details::enumeratorForMetaType(mt);
        if (metaEnum.isValid()) {
            auto* key = metaEnum.valueToKey(value.toInt());
            if (key) {
                return QString::fromLatin1(key);
            } else {
                return value.toInt();
            }
        }
    }

    // проверяем на вложенный гаджет
    if (mt.metaObject()) {
        return details::serializeGadget(value);
    }

    // ассоциативный контейнер Map Hash UnorderedHash
    if (value.canConvert<QVariantMap>() || value.canConvert<QVariantHash>()) {
        QMetaAssociation::Iterable iterable = value.value<QMetaAssociation::Iterable>();
        QVariantMap                res;
        for (auto it = iterable.begin(); it != iterable.end(); ++it) {
            QVariant keyValue = serializeValue(it.key());
            res.insert(keyValue.toString(), serializeValue(it.value()));
        }
    }

    // последовательный контейнер
    if (value.canConvert<QVariantList>()) {
        QMetaSequence::Iterable iterable = value.value<QMetaSequence::Iterable>();
        QVariantList            result;
        for (const QVariant& item : iterable)
            result.append(serializeValue(item)); // тот же fallback на QMetaType-детекцию
        return result;
    }


    // 5. Обычное примитивное значение — как есть (QString, int, double, QUuid, bool...)
    return value;
}

QVariantMap serializeGadget(const QVariant& gadgetValue) {
    QVariantMap        res;
    const QMetaObject* mo = QMetaType(gadgetValue.userType()).metaObject();
    if (!mo)
        return res;

    auto* raw = gadgetValue.constData();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        if (!prop.isReadable())
            continue;
        QVariant fieldValue = prop.readOnGadget(raw);
        res.insert(QString::fromLatin1(prop.name()), serializeValue(fieldValue));
    }
    return res;
}

QVariant deserializeValue(int targetTypeId, const QVariant& flatValue, const QMetaProperty& prop) {
    QMetaType targetType(targetTypeId); // <-- берём тип ЦЕЛИ, а не flatValue

    if (!flatValue.isValid() || flatValue.isNull())
        return QVariant(targetType);

    const bool isEnumViaProp = prop.isValid() && prop.isEnumType();
    const bool isEnumViaType = targetType.flags().testFlag(QMetaType::IsEnumeration);

    if (isEnumViaProp || isEnumViaType) {
        QMetaEnum metaEnum =
            isEnumViaProp ? prop.enumerator() : details::enumeratorForMetaType(targetType);
        int intValue = 0;
        if (metaEnum.isValid() && flatValue.userType() == QMetaType::QString) {
            bool ok  = false;
            intValue = metaEnum.keyToValue(flatValue.toString().toLatin1().constData(), &ok);
            if (!ok)
                intValue = 0;
        } else {
            intValue = flatValue.toInt();
        }

        QVariant result(targetType);
        *static_cast<int*>(result.data()) = intValue;
        return result;
    }

    // вложенный гаджет
    if (targetType.metaObject() && flatValue.userType() == QMetaType::QVariantMap) {
        return details::deserializeGadget(targetTypeId, flatValue.toMap());
    }


    QVariant result = flatValue;
    if (result.userType() != targetTypeId)
        result.convert(targetType);
    return result;
}

QVariant deserializeGadget(int targetTypeId, const QVariantMap& map) {
    QMetaType          mt(targetTypeId);
    const QMetaObject* mo = mt.metaObject();
    if (!mo)
        return QVariant();

    QVariant result(mt);
    mt.construct(result.data()); // конструируем default-инстанс нужного типа

    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        if (!prop.isWritable())
            continue;
        auto it = map.find(QString::fromLatin1(prop.name()));
        if (it == map.end())
            continue;
        QVariant fieldValue = deserializeValue(prop.userType(), it.value(), prop);
        prop.writeOnGadget(result.data(), fieldValue);
    }

    return result;
}

QVariantMap QObjectToVariantMap(const QObject* obj) {
    QVariantMap map;
    if (!obj)
        return map;

    const QMetaObject* mo = obj->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        if (prop.isReadable()) {
            map.insert(QString::fromLatin1(prop.name()), prop.read(obj));
        }
    }
    return map;
}

DeserializationResult variantMapToQObject(const QVariantMap& map, QObject* obj) {
    DeserializationResult result;
    if (!obj || map.isEmpty()) {
        return result;
    }
    result.totalKeysInMap = map.size();

    const QMetaObject* mo = obj->metaObject();
    for (const auto& [key, value] : map.asKeyValueRange()) {
        const auto str           = key.toLatin1();
        int        propertyIndex = mo->indexOfProperty(str.constData());
        if (propertyIndex == -1) {
            result.unknownKeys.append(key);
            continue;
        }
        QMetaProperty prop = mo->property(propertyIndex);

        if (!prop.isWritable()) {
            result.readOnlyKeys.append(key);
            continue;
        }

        if (prop.write(obj, value)) {
            result.setPropertiesCount++;
        } else {
            result.writeFailedKeys.append(key);
        }
    }
    result.success = (result.setPropertiesCount > 0);
    return result;
}
} // namespace details

// восстанавливает гаджет из варианта
QVariantMap readSchemeToVariant(const QSpace::IO::ReadScheme& scheme) {
    QVariantMap out;
    std::visit(
        [&](auto&& s) {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, IO::DefaultScheme>) {
                out["type"] = QStringLiteral("DefaultScheme");
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                out["type"] = QStringLiteral("None");
            } else {
                // Сработает для ColumnScheme, DefaultScheme и т.д.
                out["type"] = QString::fromLatin1(T::staticMetaObject.className());
                out["data"] = gadgetToVariantMap(s);
            }
        },
        scheme);
    return out;
}

QSpace::IO::ReadScheme variantToReadScheme(const QVariantMap& map) {
    Q_UNUSED(map)
    // const QString     type = map.value("type").toString();
    // const QVariantMap data = map.value("data").toMap();

    // if (type == QLatin1String("ColumnScheme")) {
    //     IO::ColumnScheme scheme = variantMapToGadget<IO::ColumnScheme>(data);
    //     scheme.columnsPolicy =
    //         variantToGadgetList<IO::ColumnMapping>(data["columnsPolicy"].toList());
    //     return scheme;
    // }
    // if (type == QLatin1String("HDF5ReadScheme"))
    //     return variantMapToGadget<IO::HDF5ReadScheme>(data);
    // if (type == QLatin1String("DefaultScheme"))
    //     return IO::DefaultScheme{};

    return std::monostate{};
}
} // namespace QSpace::Reflection