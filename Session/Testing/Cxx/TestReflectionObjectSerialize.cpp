#include <QObject>
#include <QVariantMap>
#include <QtTest>
#include <qtestcase.h>
#include "../TestQObjects.h"

// Подключите ваш заголовочный файл с сериализацией
#include <Session/Reflection.h>

class TestReflectionObjectSerialize : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    // 1. Тест сериализации QObject -> QVariantMap
    void testSerialization();

    // 2. Тест десериализации QVariantMap -> QObject
    void testDeserialization();

    // 3. Тест детальной информации о десериализации (DeserializationResult)
    void testDeserializationResult();

    void testPolymorphicSerialization();

    // 5. Тест полиморфной десериализации
    void testPolymorphicDeserialization();
};

void TestReflectionObjectSerialize::initTestCase() {
    qRegisterMetaType<TestTypes::Status>("TestTypes::Status");
}

void TestReflectionObjectSerialize::testSerialization() {
    TestTypes::TestObject obj;
    obj.setObjectName("MyTestObject");
    obj.setId(42);
    obj.setTitle("Project X");
    obj.setIsActive(true);

    // Сериализуем объект
    QVariantMap map = QSpace::Reflection::QObjectToVariantMap(&obj);

    // Проверяем наличие всех ключей, включая унаследованный objectName
    QVERIFY(map.contains("objectName"));
    QVERIFY(map.contains("id"));
    QVERIFY(map.contains("title"));
    QVERIFY(map.contains("isActive"));
    QVERIFY(map.contains("readOnlyField"));

    // Проверяем значения
    QCOMPARE(map.value("objectName").toString(), QString("MyTestObject"));
    QCOMPARE(map.value("id").toInt(), 42);
    QCOMPARE(map.value("title").toString(), QString("Project X"));
    QCOMPARE(map.value("isActive").toBool(), true);
    QCOMPARE(map.value("readOnlyField").toString(), QString("ConstantData"));
}

void TestReflectionObjectSerialize::testDeserialization() {
    // Подготавливаем входные данные
    QVariantMap map;
    map["objectName"] = "RestoredObject";
    map["id"]         = 99;
    map["title"]      = "Restored Project";
    map["isActive"]   = false;

    // Создаем "пустой" объект
    TestTypes::TestObject obj;
    QCOMPARE(obj.id(), 0); // Убедимся, что он инициализирован дефолтными значениями

    // Десериализуем данные в объект
    auto result = QSpace::Reflection::variantMapToQObject(map, &obj);

    // Проверяем успешность
    QVERIFY(result.success);

    // Свойства должны обновиться
    QCOMPARE(obj.objectName(), QString("RestoredObject"));
    QCOMPARE(obj.id(), 99);
    QCOMPARE(obj.title(), QString("Restored Project"));
    QCOMPARE(obj.isActive(), false);
}

void TestReflectionObjectSerialize::testDeserializationResult() {
    // Формируем карту с "проблемными" ключами
    QVariantMap map;

    // 1. Валидные ключи (2 штуки)
    map["id"]    = 100;
    map["title"] = "Some Title";

    // 2. Ключ, которого нет в объекте (unknown)
    map["nonExistentProperty"] = "Hello";
    map["fakeId"]              = 123;

    // 3. Ключ, который есть, но он только для чтения
    map["readOnlyField"] = "TryToOverwrite";

    TestTypes::TestObject obj;
    auto                  result = QSpace::Reflection::variantMapToQObject(map, &obj);

    // Успех должен быть true, так как хотя бы id и title записались
    QVERIFY(result.success);

    // Проверка счетчиков (всего 5 ключей подали на вход)
    QCOMPARE(result.totalKeysInMap, 5);
    QCOMPARE(result.setPropertiesCount, 2); // Записались только id и title

    // Проверка неизвестных ключей (должно быть 2)
    QCOMPARE(result.unknownKeys.size(), 2);
    QVERIFY(result.unknownKeys.contains("nonExistentProperty"));
    QVERIFY(result.unknownKeys.contains("fakeId"));

    // Проверка read-only ключей (должен быть 1)
    QCOMPARE(result.readOnlyKeys.size(), 1);
    QVERIFY(result.readOnlyKeys.contains("readOnlyField"));

    // Проверка функции-хелпера isFullyApplied
    QVERIFY(result.isFullyApplied() == false);
}

void TestReflectionObjectSerialize::testPolymorphicSerialization() {
    // Создаем экземпляр наследника
    TestTypes::TestObjectDerived derivedObj;
    derivedObj.setObjectName("DerivedInstance");
    derivedObj.setId(101);
    derivedObj.setTitle("Polymorphic Project");
    derivedObj.setIsActive(true);
    derivedObj.setExtraValue(42.5);
    derivedObj.setStatus(TestTypes::Status::Active);

    // ПОЛИМОРФИЗМ: Приводим к указателю на базовый класс!
    TestTypes::TestObject* basePtr = &derivedObj;

    // Сериализуем через базовый указатель
    QVariantMap map = QSpace::Reflection::QObjectToVariantMap(basePtr);

    // Проверяем, что сериализатор вытянул свойства из ВСЕЙ цепочки наследования

    // Свойства базового класса QObject
    QVERIFY(map.contains("objectName"));
    QCOMPARE(map.value("objectName").toString(), QString("DerivedInstance"));

    // Свойства базового класса TestObject
    QVERIFY(map.contains("id"));
    QVERIFY(map.contains("title"));
    QCOMPARE(map.value("id").toInt(), 101);
    QCOMPARE(map.value("title").toString(), QString("Polymorphic Project"));

    // Свойство класса-наследника TestObjectDerived
    QVERIFY(map.contains("extraValue"));
    QCOMPARE(map.value("extraValue").toDouble(), 42.5);

    QVERIFY(map.contains("status"));
    QCOMPARE(map.value("status").value<TestTypes::Status>(), TestTypes::Status::Active);
}

void TestReflectionObjectSerialize::testPolymorphicDeserialization() {
    // Подготавливаем карту, содержащую как базовые свойства, так и свойства наследника
    QVariantMap map;
    map["id"]         = 777;
    map["title"]      = "Restored Derived";
    map["extraValue"] = 99.9;
    map["status"]     = QVariant::fromValue(TestTypes::Status::Disabled);

    // Создаем экземпляр наследника
    TestTypes::TestObjectDerived derivedObj;

    // ПОЛИМОРФИЗМ: Приводим к указателю на базовый класс
    TestTypes::TestObject* basePtr = &derivedObj;

    // Десериализуем через базовый указатель
    auto result = QSpace::Reflection::variantMapToQObject(map, basePtr);

    // Убеждаемся, что десериализация прошла успешно и не выдала "unknown keys"
    QVERIFY(result.success);
    QVERIFY(result.unknownKeys.isEmpty());
    QCOMPARE(result.setPropertiesCount, 4); // id, title, extraValue

    // Проверяем, что значения реально записались в объект-наследник
    QCOMPARE(derivedObj.id(), 777);
    QCOMPARE(derivedObj.title(), QString("Restored Derived"));
    QCOMPARE(derivedObj.extraValue(), 99.9);
    QCOMPARE(derivedObj.status(), TestTypes::Status::Disabled);
}
QTEST_APPLESS_MAIN(TestReflectionObjectSerialize)

// Включаем MOC файл, так как мы объявили классы с Q_OBJECT прямо в .cpp файле
#include "TestReflectionObjectSerialize.moc"