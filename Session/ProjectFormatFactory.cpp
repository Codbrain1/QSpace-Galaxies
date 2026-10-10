#include "ProjectFormatFactory.h"

#include "JsonProjectFormatCodec.h"
#include <QFileInfo>

namespace QSpace::Session {

ProjectFormatCodecPtr ProjectFormatFactory::create(ProjectFormat format) {
    switch (format) {
    case ProjectFormat::Json:
        return std::make_unique<JsonProjectFormatCodec>();
    case ProjectFormat::Xml:
        return nullptr;
    }

    return nullptr;
}

std::optional<ProjectFormat> ProjectFormatFactory::fromFilePath(const QString& filePath) {
    const QString suffix = QFileInfo(filePath).suffix().toLower();

    if (suffix == QStringLiteral("json")) {
        return ProjectFormat::Json;
    }
    if (suffix == QStringLiteral("xml")) {
        return ProjectFormat::Xml;
    }

    return std::nullopt;
}

QString ProjectFormatFactory::extension(ProjectFormat format) {
    switch (format) {
    case ProjectFormat::Json:
        return QStringLiteral("json");
    case ProjectFormat::Xml:
        return QStringLiteral("xml");
    }

    return {};
}

QString ProjectFormatFactory::name(ProjectFormat format) {
    switch (format) {
    case ProjectFormat::Json:
        return QStringLiteral("JSON");
    case ProjectFormat::Xml:
        return QStringLiteral("XML");
    }

    return {};
}

} // namespace QSpace::Session