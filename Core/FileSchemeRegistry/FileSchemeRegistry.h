#pragma once
#include "Common/Structures/FileSchemeRegistry.h"
#include "Common/Structures/FileSchemeStructures.h"
#include <QHash>
#include <QObject>
#include <qtmetamacros.h>
#include <quuid.h>
#include "IO/SchemePresets.h"
#include <optional>

namespace QSpace::Core {
class FileSchemeRegistry : public QObject {
    Q_OBJECT
  public:
    explicit FileSchemeRegistry(QObject* parent = nullptr);

    QUuid registerReadScheme(const IO::ReadScheme& scheme, const QString& name = {});
    QUuid registerWriteScheme(const IO::WriteScheme& scheme, const QString& name = {});
    std::optional<IO::ReadScheme>  getReadScheme(const QUuid& id);
    std::optional<IO::WriteScheme> getWriteScheme(const QUuid& id);
    void                           removeReadScheme(const QUuid& id);
    void                           removeWriteScheme(const QUuid& id);
    QList<IO::ReadSchemeUI>        allReadSchemes();
    QList<IO::WriteSchemeUI>       allWriteSchemes();
    void                           clear();

    void                 registerStandartPresets();
    std::optional<QUuid> findPresetId(const IO::PresetKey& key) const;

    // --- пользовательские оверрайды ---
    void                 setUserOverride(const IO::PresetKey& key, const QUuid& schemeId);
    void                 clearUserOverride(const IO::PresetKey& key);
    std::optional<QUuid> findUserOverrideFor(const IO::PresetKey& key) const;


  signals:
    // сигналы для модели UI
    void readSchemeAdded(const QUuid& id, const QString& name);
    void writeSchemeAdded(const QUuid& id, const QString& name);
    void readSchemeRemoved(const QUuid& id);
    void writeSchemeRemoved(const QUuid& id);
    void userOverrideChanged(const IO::PresetKey& key, const QUuid& newSchemeId);
    void cleared();

  private:
    inline QUuid presetIdFor(const IO::PresetKey& key) const {
        // детерминированный UUID из содержимого ключа — тот же вход всегда даёт тот же id
        QString seed =
            QString("%1|%2|%3").arg(int(key.type)).arg(int(key.format)).arg(int(key.mpv));
        return QUuid::createUuidV5(QUuid{}, seed); // V5 — детерминированная генерация из строки
    }

    QMap<QUuid, IO::ReadSchemeUI>  m_readSchemes;
    QMap<QUuid, IO::WriteSchemeUI> m_writeSchemes;
    QHash<IO::PresetKey, QUuid>    m_userOverrideIndexReadScheme;
};
} // namespace QSpace::Core