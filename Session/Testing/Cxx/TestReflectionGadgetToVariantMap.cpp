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
    void testSimpleGadgetSerialization();
    void testComplexGadgetSerialization();

    // Data-driven тесты
    void testDataDrivenGadgets_data();
    void testDataDrivenGadgets();

    // Тесты методов схемы файла
    void testReadSchemeRoundTrip();
};

void ReflectionTest::initTestCase() {
    // Регистрируем мета-типы со строковыми именами, чтобы QVariant корректно их обрабатывал
    // и мог создавать экземпляры по имени типа при десериализации
    qRegisterMetaType<TestTypes::SimpleGadget>("TestTypes::SimpleGadget");
    qRegisterMetaType<TestTypes::ComplexGadget>("TestTypes::ComplexGadget");
    qRegisterMetaType<TestTypes::Status>("TestTypes::Status");
    qRegisterMetaType<QList<TestTypes::SimpleGadget>>("QList<TestTypes::SimpleGadget>");

    // Регистрация типов схемы для работы variantToReadScheme
    qRegisterMetaType<QSpace::IO::ColumnMapping>("QSpace::IO::ColumnMapping");
    qRegisterMetaType<QList<QSpace::IO::ColumnMapping>>("QList<QSpace::IO::ColumnMapping>");
    qRegisterMetaType<QSpace::IO::ColumnScheme>("QSpace::IO::ColumnScheme");
    qRegisterMetaType<QSpace::IO::DefaultScheme>("QSpace::IO::DefaultScheme");
}

// 1. Тест простого гаджета: Проверяем точность полей в QVariantMap
void ReflectionTest::testSimpleGadgetSerialization() {
    TestTypes::SimpleGadget original;
    original.setId(42);
    original.setName("SpaceStation");

    QVariantMap map = QSpace::Reflection::gadgetToVariantMap(original);

    QCOMPARE(map.value("id").toInt(), 42);
    QCOMPARE(map.value("name").toString(), QString("SpaceStation"));
}

// 2. Тест комплексного гаджета (вложенный гаджет + список + enum)
void ReflectionTest::testComplexGadgetSerialization() {
    TestTypes::SimpleGadget inner;
    inner.setId(100);
    inner.setName("InnerData");

    TestTypes::ComplexGadget original;
    original.setStatus(TestTypes::Status::Active);
    original.setNested(inner);

    QList<int> numbers = {1, 2, 3, 5, 8};
    original.setNumbers(numbers);

    QList<TestTypes::SimpleGadget> simpleList;
    for (int i = 0; i < 4; ++i) {
        TestTypes::SimpleGadget sg;
        sg.setId(i);
        sg.setName(QString("Gadget_%1").arg(i));
        simpleList.append(sg);
    }
    original.setSimpleGadgets(simpleList);

    QVariantMap map = QSpace::Reflection::gadgetToVariantMap(original);

    QCOMPARE(map.value("status").toString(), QString("Active"));
    QCOMPARE(map.value("nested").toMap().value("id"), 100);
    QCOMPARE(map.value("nested").toMap().value("name"), QString("InnerData"));

    QVariantList expectedVariantNumbers = {1, 2, 3, 5, 8};
    QCOMPARE(map.value("numbers").toList(), expectedVariantNumbers);

    const QVariantList variantList = map.value("simpleGadgets").toList();
    QCOMPARE(variantList.size(), simpleList.size());

    for (int i = 0; i < simpleList.size(); ++i) {
        QVariantMap gadgetMap = variantList.at(i).toMap();
        QCOMPARE(gadgetMap.value("id").toInt(), simpleList.at(i).id());
        QCOMPARE(gadgetMap.value("name").toString(), simpleList.at(i).name());
    }
}

// 3. Data-driven тест с наборами данных (полный цикл сериализации -> десериализации)
void ReflectionTest::testDataDrivenGadgets_data() {
    QTest::addColumn<int>("id");
    QTest::addColumn<QString>("name");

    QTest::newRow("Normal value") << 101 << "Apollo";
    QTest::newRow("Empty string") << 0 << "";
    QTest::newRow("Special characters") << -5 << "🛰️ Space #1 & test";
}

void ReflectionTest::testDataDrivenGadgets() {
    QFETCH(int, id);
    QFETCH(QString, name);

    TestTypes::SimpleGadget original;
    original.setId(id);
    original.setName(name);

    QVariantMap map      = QSpace::Reflection::gadgetToVariantMap(original);
    auto        restored = QSpace::Reflection::variantMapToGadget<TestTypes::SimpleGadget>(map);

    QCOMPARE(restored.id(), id);
    QCOMPARE(restored.name(), name);
    // Проверка оператора ==
    QVERIFY(original == restored);
}

// 4. Тест функций работы со схемой файла
void ReflectionTest::testReadSchemeRoundTrip() {
    // Подготовка сложной тестовой схемы с колонками
    QSpace::IO::ColumnScheme columnScheme;
    columnScheme.headerOffsetBytes = 256;
    columnScheme.isInterleaved     = true;
    columnScheme.delimiter         = ",";

    QSpace::IO::ColumnMapping mapX;
    mapX.name               = "CoordinateX";
    mapX.vtkDataType        = 10;
    mapX.vtkAttributeRole   = 1;
    mapX.numberOfComponents = 1;
    mapX.isCoordinate       = 0; // Координата X

    QSpace::IO::ColumnMapping mapDensity;
    mapDensity.name               = "Density";
    mapDensity.vtkDataType        = 11;
    mapDensity.vtkAttributeRole   = 2;
    mapDensity.numberOfComponents = 1;
    mapDensity.isCoordinate       = -1; // Не координата

    columnScheme.columnsPolicy.append(mapX);
    columnScheme.columnsPolicy.append(mapDensity);

    // Заворачиваем в std::variant
    QSpace::IO::ReadScheme originalScheme = columnScheme;

    // Сериализация (запись)
    QVariantMap map = QSpace::Reflection::readSchemeToVariant(originalScheme);

    // Проверка корректности формирования верхней структуры ("type" и "data")
    QVERIFY(!map.isEmpty());
    QCOMPARE(map.value("type").toString(), QString("ColumnScheme"));
    QVERIFY(map.contains("data"));
    QCOMPARE(map.value("data").toMap().value("delimiter").toString(), QString(","));

    // Десериализация (чтение)
    QSpace::IO::ReadScheme restoredScheme = QSpace::Reflection::variantToReadScheme(map);

    // Убеждаемся, что внутри variant лежит нужный тип
    QVERIFY(std::holds_alternative<QSpace::IO::ColumnScheme>(restoredScheme));

    // Сравниваем исходную схему и восстановленную с помощью operator==,
    // который был сгенерирован через = default
    const auto& restoredColumnScheme = std::get<QSpace::IO::ColumnScheme>(restoredScheme);
    QVERIFY(columnScheme == restoredColumnScheme);
}

QTEST_APPLESS_MAIN(ReflectionTest)
#include "TestReflectionGadgetToVariantMap.moc"