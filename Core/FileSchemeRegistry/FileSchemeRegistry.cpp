#include "FileSchemeRegistry.h"
#include "Common/Logger/Logger.h"
#include <qobject.h>
#include "IO/SchemePresets.h"
#include "Structures/FileSchemeStructures.h"
#include <optional>

namespace QSpace::Core {
FileSchemeRegistry::FileSchemeRegistry(QObject* parent) : QObject(parent) {
}

void FileSchemeRegistry::registerStandartPresets() {
    for (const auto& [key, scheme] : IO::ReadSchemePresets::getStandardPresets()) {
        QUuid id = presetIdFor(key);
        m_readSchemes.insert(id,
                             IO::ReadSchemeUI{id,
                                              IO::ReadSchemePresets::presetDisplayName(key),
                                              /*isBuiltIn=*/true,
                                              scheme});
        emit readSchemeAdded(id, m_readSchemes[id].name);
    }
}

QUuid FileSchemeRegistry::registerReadScheme(const IO::ReadScheme& scheme, const QString& name) {
    QUuid            id = QUuid::createUuid();
    IO::ReadSchemeUI schemeui;
    schemeui.scheme    = scheme;
    schemeui.id        = id;
    schemeui.isBuiltIn = false;
    schemeui.name      = name;

    m_readSchemes.insert(id, schemeui);
    emit readSchemeAdded(id, name);
    return id;
}

QUuid FileSchemeRegistry::registerWriteScheme(const IO::WriteScheme& scheme, const QString& name) {
    QUuid             id = QUuid::createUuid();
    IO::WriteSchemeUI schemeui;
    schemeui.scheme    = scheme;
    schemeui.id        = id;
    schemeui.isBuiltIn = false;
    schemeui.name      = name;

    m_writeSchemes.insert(id, schemeui);
    emit writeSchemeAdded(id, name);
    return id;
}

std::optional<IO::ReadScheme> FileSchemeRegistry::getReadScheme(const QUuid& id) {
    auto it = m_readSchemes.find(id);
    if (it != m_readSchemes.end())
        return it->scheme;
    else
        return std::nullopt;
}

std::optional<IO::WriteScheme> FileSchemeRegistry::getWriteScheme(const QUuid& id) {
    auto it = m_writeSchemes.find(id);
    if (it != m_writeSchemes.end())
        return it->scheme;
    else
        return std::nullopt;
}

void FileSchemeRegistry::removeReadScheme(const QUuid& id) {
    if (m_readSchemes.remove(id))
        emit readSchemeRemoved(id);

    // убрать все оверрайды, указывавшие на удалённую схему
    for (auto it = m_userOverrideIndexReadScheme.begin();
         it != m_userOverrideIndexReadScheme.end();) {
        if (it.value() == id)
            it = m_userOverrideIndexReadScheme.erase(it);
        else
            ++it;
    }
}

void FileSchemeRegistry::removeWriteScheme(const QUuid& id) {
    if (m_writeSchemes.remove(id))
        emit writeSchemeRemoved(id);
}

QList<IO::ReadSchemeUI> FileSchemeRegistry::allReadSchemes() {
    return m_readSchemes.values();
}

QList<IO::WriteSchemeUI> FileSchemeRegistry::allWriteSchemes() {
    return m_writeSchemes.values();
}

void FileSchemeRegistry::clear() {
    m_readSchemes.clear();
    m_writeSchemes.clear();
    emit cleared();
}

std::optional<QUuid> FileSchemeRegistry::findPresetId(const IO::PresetKey& key) const {
    QUuid id = presetIdFor(key);
    if (m_readSchemes.contains(id) && m_readSchemes[id].isBuiltIn)
        return id;
    return std::nullopt;
}

void FileSchemeRegistry::setUserOverride(const IO::PresetKey& key, const QUuid& schemeId) {
    // проверяем, что схема с таким id реально существует —
    // либо как пользовательская, либо как встроенный пресет
    if (!m_readSchemes.contains(schemeId)) {
        qCWarning(LogCore)
            << "FileSchemeRegistry: попытка назначить оверрайд на несуществующую схему:"
            << schemeId;
        return;
    }

    m_userOverrideIndexReadScheme.insert(key, schemeId);
    emit userOverrideChanged(key, schemeId);
}

void FileSchemeRegistry::clearUserOverride(const IO::PresetKey& key) {
    if (m_userOverrideIndexReadScheme.remove(key) > 0) {
        emit userOverrideChanged(key, QUuid()); // пустой QUuid = "оверрайд снят, используем пресет"
    }
}

std::optional<QUuid> FileSchemeRegistry::findUserOverrideFor(const IO::PresetKey& key) const {
    auto it = m_userOverrideIndexReadScheme.find(key);
    if (it != m_userOverrideIndexReadScheme.end())
        return it.value();
    return std::nullopt;
}
} // namespace QSpace::Core