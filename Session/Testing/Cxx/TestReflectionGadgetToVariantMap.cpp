#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>
#include "../TestGadgets.h"
#include <Session/Reflection.h>

class ReflectionTest : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase();
    void testEmptyMapAndDefaults();
    void testQtValueTypes();
    void testNestedDtoRoundTrip();
    void testNullValuesAndMissingFields();
    void testLegacyJsonCompatibility();
};

void ReflectionTest::initTestCase() {
    qRegisterMetaType<TestTypes::SimpleGadget>("TestTypes::SimpleGadget");
    qRegisterMetaType<TestTypes::ComplexGadget>("TestTypes::ComplexGadget");
    qRegisterMetaType<TestTypes::QtValueGadget>("TestTypes::QtValueGadget");
    qRegisterMetaType<TestTypes::Status>("TestTypes::Status");
    qRegisterMetaType<QList<TestTypes::SimpleGadget>>("QList<TestTypes::SimpleGadget>");
    qRegisterMetaType<QHash<QString, int>>("QHash<QString, int>");
}

void ReflectionTest::testEmptyMapAndDefaults() {
    const auto restored = QSpace::Reflection::variantMapToGadget<TestTypes::ComplexGadget>({});
    QCOMPARE(restored.status(), TestTypes::Status::Unknown);
    QCOMPARE(restored.nested().id(), 0);
    QVERIFY(restored.numbers().isEmpty());

    TestTypes::SimpleGadget gadget;
    const auto result = QSpace::Reflection::variantMapToGadget<TestTypes::SimpleGadget>({});
    QVERIFY(result == gadget);
}

void ReflectionTest::testQtValueTypes() {
    TestTypes::QtValueGadget original;
    const auto uuid = QUuid::createUuid();
    const auto timestamp = QDateTime::fromString("2026-10-24T02:49:43Z", Qt::ISODate);
    original.setUuid(uuid);
    original.setColor(QColor("#123456"));
    original.setTimestamp(timestamp);
    original.setValues({{"one", 1}, {"two", 2}});
    original.setIds({uuid, QUuid::createUuid()});

    const auto map = QSpace::Reflection::gadgetToVariantMap(original);
    QCOMPARE(map.value("uuid").toUuid(), uuid);
    QCOMPARE(map.value("color").value<QColor>(), QColor("#123456"));
    QCOMPARE(map.value("timestamp").toDateTime(), timestamp);
    QCOMPARE(map.value("values").toMap().value("two").toInt(), 2);
    QCOMPARE(map.value("ids").toList().size(), 2);

    const auto restored = QSpace::Reflection::variantMapToGadget<TestTypes::QtValueGadget>(map);
    QCOMPARE(restored.uuid(), uuid);
    QCOMPARE(restored.color(), QColor("#123456"));
    QCOMPARE(restored.timestamp(), timestamp);
    QCOMPARE(restored.values(), original.values());
    QCOMPARE(restored.ids(), original.ids());
}

void ReflectionTest::testNestedDtoRoundTrip() {
    TestTypes::ComplexGadget original;
    TestTypes::SimpleGadget nested;
    nested.setId(17);
    nested.setName("nested DTO");
    original.setStatus(TestTypes::Status::Active);
    original.setNested(nested);
    original.setNumbers({1, 2, 3});

    const auto map = QSpace::Reflection::gadgetToVariantMap(original);
    QVERIFY(map.value("nested").toMap().contains("id"));
    const auto restored = QSpace::Reflection::variantMapToGadget<TestTypes::ComplexGadget>(map);
    QCOMPARE(restored.status(), original.status());
    QCOMPARE(restored.nested(), original.nested());
    QCOMPARE(restored.numbers(), original.numbers());
}

void ReflectionTest::testNullValuesAndMissingFields() {
    QVariantMap map;
    map.insert("id", QVariant());
    map.insert("name", QString("kept"));
    const auto restored = QSpace::Reflection::variantMapToGadget<TestTypes::SimpleGadget>(map);
    QCOMPARE(restored.id(), 0);
    QCOMPARE(restored.name(), QString("kept"));

    const auto defaults = QSpace::Reflection::variantMapToGadget<TestTypes::QtValueGadget>({});
    QVERIFY(defaults.uuid().isNull());
    QVERIFY(!defaults.timestamp().isValid());
}

void ReflectionTest::testLegacyJsonCompatibility() {
    const QJsonObject legacy{{"id", 42}, {"name", "legacy"}};
    const auto map = QJsonDocument(legacy).object().toVariantMap();
    const auto restored = QSpace::Reflection::variantMapToGadget<TestTypes::SimpleGadget>(map);
    QCOMPARE(restored.id(), 42);
    QCOMPARE(restored.name(), QString("legacy"));
}

QTEST_APPLESS_MAIN(ReflectionTest)
#include "TestReflectionGadgetToVariantMap.moc"
