#pragma once

#include "Common/Structures/SessionStructures.h"
#include <QString>
#include <optional>

namespace QSpace::Session {

class ProjectFormatFactory {
  public:
    static std::optional<ProjectFormat> fromFilePath(const QString& filePath);
    static QString                     extension(ProjectFormat format);
    static QString                     name(ProjectFormat format);
};

} // namespace QSpace::Session
