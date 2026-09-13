#include <QObject>
#include <QtTest>
#include "../TestGadgets.h"

// Подключите ваш заголовочный файл с сериализацией
#include <Session/Reflection.h>

class ReflectionTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();

    // Тесты базовых шаблонов
    void testSimpleGadgetSerialization();  // тест сериализации простого гаджета в variantmap
    void testComplexGadgetSerialization(); // тест сериализации сложного гаджета в variantmap
    // void testDataDrivenGadgets_data();
    // void testDataDrivenGadgets();

    // Тесты методов схемы файла
    // void testReadSchemeRoundTrip();
};

void ReflectionTest::initTestCase() {
    // Регистрируем мета-типы, чтобы QVariant корректно их обрабатывал
    // qRegisterMetaType<TestTypes::SimpleGadget>();
    // qRegisterMetaType<TestTypes::ComplexGadget>();
    // qRegisterMetaType<TestTypes::Status>();
}

// 1. Тест простого гаджета: Проверяем точность полей в QVariantMap
void ReflectionTest::testSimpleGadgetSerialization() {
    TestTypes::SimpleGadget original;
    original.setId(42);
    original.setName("SpaceStation");

    QVariantMap map = QSpace::Reflection::gadgetToVariantMap(original);

    qDebug() << "Serialized QVariantMap:" << map;
    QCOMPARE(map.value("id").toInt(), 42);
    QCOMPARE(map.value("name").toString(), QString("SpaceStation"));
}

// 2. Тест комплексного гаджета (вложенный гаджет + список + enum)
void ReflectionTest::testComplexGadgetSerialization() {
    // создаем простой вложенный гаджет
    TestTypes::SimpleGadget inner;
    inner.setId(100);
    inner.setName("InnerData");

    // создаем комплексный гаджет
    TestTypes::ComplexGadget original;
    original.setStatus(TestTypes::Status::Active); // добавляем  enum
    original.setNested(inner);                     // добавляем вложенный гаджет
    QList<int> numbers = {1, 2, 3, 5, 8};
    original.setNumbers(numbers); // добавляем коллекцию простых типов

    QList<TestTypes::SimpleGadget> simpleList;
    for (int i = 0; i < 4; ++i) {
        TestTypes::SimpleGadget sg;
        sg.setId(i);
        sg.setName(QString("Gadget_%1").arg(i));
        simpleList.append(sg);
    }
    original.setSimpleGadgets(simpleList); // добавляем список простых гаджетов

    // Gadget -> VariantMap
    QVariantMap map = QSpace::Reflection::gadgetToVariantMap(original);
    qDebug() << "Serialized ComplexGadget QVariantMap:" << map;
    QCOMPARE(map.value("status").toString(), QString("Active"));
    QCOMPARE(map.value("nested").toMap().value("id"), 100);
    QCOMPARE(map.value("nested").toMap().value("name"), QString("InnerData"));
    QVariantList expectedVariantNumbers = {1, 2, 3, 5, 8};

    // map.value("numbers").toList() возвращает QVariantList
    QCOMPARE(map.value("numbers").toList(), expectedVariantNumbers);

    // QCOMPARE(map.value("simpleGadgets").toList(), simpleList);
}

// // 3. Data-driven тест с наборами данных
// void ReflectionTest::testDataDrivenGadgets_data() {
//     QTest::addColumn<int>("id");
//     QTest::addColumn<QString>("name");

//     QTest::newRow("Normal value") << 101 << "Apollo";
//     QTest::newRow("Empty string") << 0 << "";
//     QTest::newRow("Special characters") << -5 << "🛰️ Space #1 & test";
// }

// void ReflectionTest::testDataDrivenGadgets() {
//     QFETCH(int, id);
//     QFETCH(QString, name);

//     TestTypes::SimpleGadget original;
//     original.setId(id);
//     original.setName(name);

//     QVariantMap map      = QSpace::Reflection::gadgetToVariantMap(original);
//     auto        restored = QSpace::Reflection::variantMapToGadget<TestTypes::SimpleGadget>(map);

//     QCOMPARE(restored.id(), id);
//     QCOMPARE(restored.name(), name);
// }

// // 4. Тест функций работы со схемой файла
// void ReflectionTest::testReadSchemeRoundTrip() {
//     QSpace::IO::ReadScheme originalScheme;
//     // Заполните структуру originalScheme необходимыми тестовыми данными
//     // originalScheme.someProperty = ...

//     QVariantMap map = QSpace::Reflection::readSchemeToVariant(originalScheme);

//     // Убедимся, что карта не пуста после конвертации
//     QVERIFY(!map.isEmpty());

//     QSpace::IO::ReadScheme restoredScheme = QSpace::Reflection::variantToReadScheme(map);

//     // Сравниваем поля исходной и восстановленной схемы
//     // QCOMPARE(restoredScheme.field, originalScheme.field);
// }

QTEST_MAIN(ReflectionTest)
#include "TestReflectionGadgetToVariantMap.moc"