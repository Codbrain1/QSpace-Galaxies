#pragma once

#include "Common/Interfaces/IProjectFormatCodec.h"
#include "Common/Structures/SessionStructures.h"
#include <QString>
#include <optional>


namespace QSpace::Session {

class ProjectFormatFactory {
  public:
    static ProjectFormatCodecPtr        create(ProjectFormat format);
    static std::optional<ProjectFormat> fromFilePath(const QString& filePath);
    static QString                      extension(ProjectFormat format);
    static QString                      name(ProjectFormat format);
};

} // namespace QSpace::Session
