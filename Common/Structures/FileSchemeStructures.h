#pragma once
#include "Common/Enums/IOEnums.h"
#include "Common/Enums/VisualizeBaseEnums.h"
#include <QList>
#include <QMap>
#include <QString>
#include <qcontainerfwd.h>

namespace QSpace::IO {
struct ColumnMapping {
    // регистрируем поля в метасистеме qt
    Q_GADGET

    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int vtkDataType MEMBER vtkDataType)
    Q_PROPERTY(int vtkAttributeRole MEMBER vtkAttributeRole)
    Q_PROPERTY(int numberOfComponents MEMBER numberOfComponents)
    Q_PROPERTY(bool isCoordinate MEMBER isCoordinate)

  public:
    int     vtkDataType; // тип данных из vtk
    int     vtkAttributeRole;
    int     numberOfComponents;
    bool    isCoordinate; // 0 для X, 1 для Y, 2 для Z, и -1 если не координата
    QString name;         // название столбца (например density)

    bool operator==(const ColumnMapping&) const = default;
};

struct ColumnScheme {
    Q_GADGET
    // Чтобы Qt понял QList<Mapping>, тип внутри свойства должен быть полностью квалифицирован
    Q_PROPERTY(QList<QSpace::IO::ColumnMapping> columnsPolicy MEMBER columnsPolicy)
    Q_PROPERTY(int headerOffsetBytes MEMBER headerOffsetBytes)
    Q_PROPERTY(bool isInterleaved MEMBER isInterleaved)
    Q_PROPERTY(QString delimiter MEMBER delimiter)

  public:
    QList<ColumnMapping> columnsPolicy;
    int                  headerOffsetBytes = 0;     // смещение относительно заголовка
    bool                 isInterleaved     = false; // true если X1,Y1,Z1, X2,Y2,Z2.
    QString              delimiter         = " ";   // для TXT файла
    bool                 operator==(const ColumnScheme&) const = default;
};

struct DefaultScheme {
    Q_GADGET
  public:
    bool operator==(const DefaultScheme&) const = default;
}; // для VTK и GRD нет определенной схемы так как имеют строгий неизменный формат

using ReadScheme  = std::variant<std::monostate, ColumnScheme, DefaultScheme>;
using WriteScheme = std::variant<std::monostate, ColumnScheme, DefaultScheme>;

struct BatchTask { // одна задача для пакетного чтения
    QString    path;
    ReadScheme scheme;
};

struct PresetKey {
    Visualize::EntityType       type;
    IO::FileFormat              format;
    IO::ModelingProgrammVersion mpv;

    bool operator==(const PresetKey&) const = default;

    friend size_t qHash(const PresetKey& k, size_t seed = 0) noexcept {
        return qHashMulti(seed, int(k.type), int(k.format), int(k.mpv));
    }
};
} // namespace QSpace::IO

// Регистрируем типы, чтобы QVariant их знал
Q_DECLARE_METATYPE(QSpace::IO::ColumnMapping)
Q_DECLARE_METATYPE(QSpace::IO::ColumnScheme)
Q_DECLARE_METATYPE(QSpace::IO::DefaultScheme)