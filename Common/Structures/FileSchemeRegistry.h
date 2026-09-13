#pragma once
#include "FileSchemeStructures.h"

namespace QSpace::IO {

// структуры для вывода в UI
struct ReadSchemeUI {
    Q_GADGET
    Q_PROPERTY(QUuid id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(bool isBuiltIn MEMBER isBuiltIn)
    Q_PROPERTY(QSpace::IO::ReadScheme scheme MEMBER scheme)

  public:
    QUuid                  id;
    QString                name;
    bool                   isBuiltIn = false;
    QSpace::IO::ReadScheme scheme;

    bool operator==(const ReadSchemeUI&) const = default;
};

struct WriteSchemeUI {
    QUuid                   id;                // уникальный идентификатор схемы
    QString                 name;              // название схемы
    bool                    isBuiltIn = false; // является ли схема стандартной
    QSpace::IO::WriteScheme scheme;
    bool                    operator==(const WriteSchemeUI&) const = default;
};

} // namespace QSpace::IO
