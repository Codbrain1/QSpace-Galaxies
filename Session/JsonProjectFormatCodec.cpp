#include "JsonProjectFormatCodec.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace QSpace::Session {

QByteArray JsonProjectFormatCodec::encode(const QVariantMap& data) const {
    return QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented);
}

std::optional<QVariantMap> JsonProjectFormatCodec::decode(const QByteArray& data) const {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }

    return document.toVariant().toMap();
}

} // namespace QSpace::Session
