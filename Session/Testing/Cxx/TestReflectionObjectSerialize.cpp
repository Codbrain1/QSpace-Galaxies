#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QtTest>
#include "../TestQObjects.h"
#include <Session/Reflection.h>

class TestReflectionObjectSerialize : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase();
    void testEmptyMapAndDefaults();
    void testQtValueTypesAndNestedObject();
    void testUnknownMissingAndReadOnlyFields();
    void testWrongFieldTypes();
    void testRoundTripAndLegacyJson();
};

void TestReflectionObjectSerialize::initTestCase() {
    qRegisterMetaType<TestTypes::Status>("TestTypes::Status");
    qRegisterMetaType<QList<int>>("QList<int>");
    qRegisterMetaType<QHash<QString, int>>("QHash<QString, int>");
}

void TestReflectionObjectSerialize::testEmptyMapAndDefaults() {
    TestTypes::TestObject object;
    const auto result = QSpace::Reflection::variantMapToQObject({}, &object);
    QVERIFY(!result.success);
    QCOMPARE(result.totalKeysInMap, 0);
    QCOMPARE(object.id(), 0);
    QVERIFY(object.title().isEmpty());
    QCOMPARE(object.isActive(), false);
}

void TestReflectionObjectSerialize::testQtValueTypesAndNestedObject() {
    TestTypes::TestValueObject object;
    const auto uuid = QUuid::createUuid();
    const auto date = QDateTime::fromString("2026-10-24T02:49:43Z", Qt::ISODate);
    object.setUuid(uuid);
    object.setStatus(TestTypes::Status::Active);
    object.setNumbers({2, 4, 8});
    object.setValues({{"alpha", 10}});
    object.setColor(QColor("#abcdef"));
    object.setTimestamp(date);

    const auto map = QSpace::Reflection::QObjectToVariantMap(&object);
    QCOMPARE(map.value("uuid").toUuid(), uuid);
    QCOMPARE(map.value("status").toString(), QString("Active"));
    QCOMPARE(map.value("numbers").toList().size(), 3);
    QCOMPARE(map.value("values").toMap().value("alpha").toInt(), 10);
    QCOMPARE(map.value("color").value<QColor>(), QColor("#abcdef"));
    QCOMPARE(map.value("timestamp").toDateTime(), date);

    TestTypes::TestValueObject restored;
    const auto result = QSpace::Reflection::variantMapToQObject(map, &restored);
    QVERIFY(result.success);
    QCOMPARE(restored.uuid(), uuid);
    QCOMPARE(restored.status(), TestTypes::Status::Active);
    QCOMPARE(restored.numbers(), QList<int>({2, 4, 8}));
    const QHash<QString, int> expectedValues{{"alpha", 10}};
    QCOMPARE(restored.values(), expectedValues);
    QCOMPARE(restored.color(), QColor("#abcdef"));
    QCOMPARE(restored.timestamp(), date);
}

void TestReflectionObjectSerialize::testUnknownMissingAndReadOnlyFields() {
    TestTypes::TestObject object;
    QVariantMap map{{"id", 10}, {"unknown", 1}, {"readOnlyField", "changed"}};
    const auto result = QSpace::Reflection::variantMapToQObject(map, &object);
    QVERIFY(result.success);
    QCOMPARE(result.setPropertiesCount, 1);
    QVERIFY(result.unknownKeys.contains("unknown"));
    QVERIFY(result.readOnlyKeys.contains("readOnlyField"));
    QVERIFY(!result.isFullyApplied());
    QCOMPARE(object.title(), QString());
    QCOMPARE(object.isActive(), false);
}

void TestReflectionObjectSerialize::testWrongFieldTypes() {
    TestTypes::TestObject object;
    QVariantMap map{{"id", QString("not an integer")}, {"isActive", QString("not bool")}};
    const auto result = QSpace::Reflection::variantMapToQObject(map, &object);
    QVERIFY(result.success);
    QCOMPARE(object.id(), 0);
}

void TestReflectionObjectSerialize::testRoundTripAndLegacyJson() {
    TestTypes::TestObjectDerived original;
    original.setObjectName("round-trip");
    original.setId(77);
    original.setTitle("DTO");
    original.setIsActive(true);
    original.setExtraValue(4.5);
    original.setStatus(TestTypes::Status::Disabled);

    const auto map = QSpace::Reflection::QObjectToVariantMap(&original);
    TestTypes::TestObjectDerived restored;
    const auto result = QSpace::Reflection::variantMapToQObject(map, &restored);
    QVERIFY(result.success);
    QCOMPARE(restored.objectName(), original.objectName());
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.title(), original.title());
    QCOMPARE(restored.isActive(), original.isActive());
    QCOMPARE(restored.extraValue(), original.extraValue());
    QCOMPARE(restored.status(), original.status());

    QVariantMap legacy{{"id", 12}, {"title", "old JSON"}, {"isActive", true}};
    TestTypes::TestObject legacyObject;
    QVERIFY(QSpace::Reflection::variantMapToQObject(legacy, &legacyObject).success);
    QCOMPARE(legacyObject.id(), 12);
    QCOMPARE(legacyObject.title(), QString("old JSON"));
}

QTEST_APPLESS_MAIN(TestReflectionObjectSerialize)
#include "TestReflectionObjectSerialize.moc"
