#include <QSignalSpy>
#include <QtTest>

#include "Core/FileSchemeRegistry/FileSchemeRegistry.h"

Q_DECLARE_METATYPE(QSpace::IO::PresetKey)

using namespace QSpace;
using namespace QSpace::Core;

class TestFileSchemeRegistry : public QObject {
    Q_OBJECT

  private:
    static IO::ColumnScheme customScheme() {
        IO::ColumnScheme scheme;
        scheme.delimiter         = ",";
        scheme.headerOffsetBytes = 16;
        scheme.isInterleaved     = true;
        scheme.columnsPolicy.append({1, 2, 3, false, "density"});
        return scheme;
    }

  private slots:
    void initTestCase() {
        qRegisterMetaType<IO::PresetKey>();
    }

    void registersStandardPresetsAndFindsThem() {
        FileSchemeRegistry registry;
        QSignalSpy addedSpy(&registry, &FileSchemeRegistry::readSchemeAdded);
        const auto presets = IO::ReadSchemePresets::getStandardPresets();

        registry.registerStandartPresets();

        QCOMPARE(addedSpy.count(), presets.size());
        QCOMPARE(registry.allReadSchemes().size(), presets.size());
        for (const auto& [key, scheme] : presets) {
            const auto id = registry.findPresetId(key);
            QVERIFY(id.has_value());

            const auto storedScheme = registry.getReadScheme(*id);
            QVERIFY(storedScheme.has_value());
            QCOMPARE(*storedScheme, scheme);

            bool foundBuiltInScheme = false;
            for (const auto& registeredScheme : registry.allReadSchemes()) {
                if (registeredScheme.id == *id) {
                    QVERIFY(registeredScheme.isBuiltIn);
                    foundBuiltInScheme = true;
                    break;
                }
            }
            QVERIFY(foundBuiltInScheme);

            const auto registered = addedSpy.takeFirst();
            QCOMPARE(registered.at(0).toUuid(), *id);
            QCOMPARE(registered.at(1).toString(), IO::ReadSchemePresets::presetDisplayName(key));
        }
    }

    void registersCustomReadAndWriteSchemesAndEmitsSignals() {
        FileSchemeRegistry registry;
        QSignalSpy readAddedSpy(&registry, &FileSchemeRegistry::readSchemeAdded);
        QSignalSpy writeAddedSpy(&registry, &FileSchemeRegistry::writeSchemeAdded);
        QSignalSpy readRemovedSpy(&registry, &FileSchemeRegistry::readSchemeRemoved);
        QSignalSpy writeRemovedSpy(&registry, &FileSchemeRegistry::writeSchemeRemoved);
        const IO::ColumnScheme scheme = customScheme();

        const QUuid readId = registry.registerReadScheme(scheme, "custom read");
        const QUuid writeId = registry.registerWriteScheme(scheme, "custom write");

        QVERIFY(!readId.isNull());
        QVERIFY(!writeId.isNull());
        QCOMPARE(*registry.getReadScheme(readId), IO::ReadScheme{scheme});
        QCOMPARE(*registry.getWriteScheme(writeId), IO::WriteScheme{scheme});
        QCOMPARE(registry.allReadSchemes().front().name, QString("custom read"));
        QCOMPARE(registry.allWriteSchemes().front().name, QString("custom write"));
        const auto readAdded = readAddedSpy.takeFirst();
        const auto writeAdded = writeAddedSpy.takeFirst();
        QCOMPARE(readAdded.at(0).toUuid(), readId);
        QCOMPARE(readAdded.at(1).toString(), QString("custom read"));
        QCOMPARE(writeAdded.at(0).toUuid(), writeId);
        QCOMPARE(writeAdded.at(1).toString(), QString("custom write"));

        registry.removeReadScheme(readId);
        registry.removeWriteScheme(writeId);

        QCOMPARE(readRemovedSpy.takeFirst().at(0).toUuid(), readId);
        QCOMPARE(writeRemovedSpy.takeFirst().at(0).toUuid(), writeId);
        QVERIFY(!registry.getReadScheme(readId).has_value());
        QVERIFY(!registry.getWriteScheme(writeId).has_value());
    }

    void managesUserOverridesAndCleansThemUpWhenSchemeIsRemoved() {
        FileSchemeRegistry registry;
        QSignalSpy overrideSpy(&registry, &FileSchemeRegistry::userOverrideChanged);
        const IO::PresetKey key{Visualize::EntityType::Gas,
                                IO::FileFormat::BIN,
                                IO::ModelingProgrammVersion::V2};
        const QUuid schemeId = registry.registerReadScheme(customScheme(), "override source");

        registry.setUserOverride(key, schemeId);

        const auto overrideId = registry.findUserOverrideFor(key);
        QVERIFY(overrideId.has_value());
        QCOMPARE(*overrideId, schemeId);
        QCOMPARE(overrideSpy.takeFirst().at(1).toUuid(), schemeId);

        registry.clearUserOverride(key);

        QVERIFY(!registry.findUserOverrideFor(key).has_value());
        QVERIFY(overrideSpy.takeFirst().at(1).toUuid().isNull());

        registry.setUserOverride(key, schemeId);
        registry.removeReadScheme(schemeId);

        QVERIFY(!registry.findUserOverrideFor(key).has_value());
    }

    void clearRemovesSchemesOverridesAndEmitsSignal() {
        FileSchemeRegistry registry;
        QSignalSpy clearedSpy(&registry, &FileSchemeRegistry::cleared);
        const IO::PresetKey key{Visualize::EntityType::Stars,
                                IO::FileFormat::BIN,
                                IO::ModelingProgrammVersion::V2_3};
        const QUuid readId = registry.registerReadScheme(customScheme(), "read");
        const QUuid writeId = registry.registerWriteScheme(customScheme(), "write");
        registry.setUserOverride(key, readId);

        registry.clear();

        QCOMPARE(clearedSpy.count(), 1);
        QVERIFY(registry.allReadSchemes().isEmpty());
        QVERIFY(registry.allWriteSchemes().isEmpty());
        QVERIFY(!registry.getReadScheme(readId).has_value());
        QVERIFY(!registry.getWriteScheme(writeId).has_value());
        QVERIFY(!registry.findUserOverrideFor(key).has_value());
    }
};

QTEST_APPLESS_MAIN(TestFileSchemeRegistry)
#include "TestFileSchemeRegistry.moc"
