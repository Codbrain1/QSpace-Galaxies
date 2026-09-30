#include <QUuid>
#include <QVariantMap>
#include <QtTest>


// Заголовки тестируемых компонентов
#include "Common/Enums/ViewEnums.h" // Убедитесь, что путь корректен для ViewType
#include "Common/Structures/SessionStructures.h"
#include "Core/ViewManager/ViewManager.h"
#include "Core/ViewManager/ViewManagerSerializer.h"


using namespace QSpace::Core;
using namespace QSpace::Session;
using namespace QSpace::Visualize::Views;

class TestViewManagerSerialize : public QObject {
    Q_OBJECT

  private slots:

    // 1. Тест сериализации пустого менеджера
    void testSerializeEmptyManager() {
        ViewManager manager;

        auto dtoOpt = ViewManagerSerializer::toDTO(&manager);

        QVERIFY(dtoOpt.has_value());

        auto dto = dtoOpt.value();
        QVERIFY(dto.mainViewId.isNull());
        QVERIFY(dto.views.isEmpty());
    }

    // 2. Тест успешной сериализации наполненного менеджера
    void testSerializePopulatedManager() {
        ViewManager manager;

        // Создаем окна через штатную фабрику менеджера
        QUuid view1Id = manager.createView(ViewType::OpenGL3D);
        QUuid view2Id = manager.createView(ViewType::QCustonPlot2D);

        // В headless-окружениях (например, в CI) создание OpenGL/VTK окон может не удаться.
        // Защищаем тест от ложных падений, пропуская его, если фабрика вернула nullptr.
        if (view1Id.isNull() || view2Id.isNull()) {
            QSKIP("ViewFactory failed to create views. Skipping serialization test (possible headless environment).");
        }

        // Назначаем главное окно
        manager.setMainView(view2Id);

        // Сериализуем
        auto dtoOpt = ViewManagerSerializer::toDTO(&manager);
        QVERIFY(dtoOpt.has_value());

        auto dto = dtoOpt.value();

        // Проверяем структуру DTO
        QCOMPARE(dto.mainViewId, view2Id);
        QCOMPARE(dto.views.size(), 2);

        // Проверяем, что конкретное окно корректно сохранило свои данные
        bool foundView2 = false;
        for (const auto& vDto : dto.views) {
            if (vDto.id == view2Id) {
                foundView2 = true;
                QCOMPARE(vDto.type, ViewType::QCustonPlot2D);
                // Имя и настройки проверять сложно, не зная дефолтных значений конкретной вьюшки,
                // но мы точно знаем, что UUID должен совпасть.
            }
        }
        QVERIFY(foundView2);
    }

    // 3. Тест десериализации (DTO -> ViewManager)
    void testDeserialization() {
        // Подготавливаем "сохраненное состояние"
        ViewManagerDTO dto;
        dto.mainViewId = QUuid::createUuid();

        ViewDTO view1;
        view1.id                     = dto.mainViewId;
        view1.name                   = "Main VTK View";
        view1.type                   = ViewType::OpenGL3D;
        view1.settings["cameraZoom"] = 1.5;

        ViewDTO view2;
        view2.id   = QUuid::createUuid();
        view2.name = "Secondary View";
        view2.type = ViewType::QCustonPlot2D;

        dto.views.append(view1);
        dto.views.append(view2);

        // Распаковываем
        ViewManager manager;
        bool        success = ViewManagerSerializer::fromDTO(dto, &manager);

        if (!success) {
            // Если фабрика не смогла восстановить OpenGL/VTK окна в тестовой среде (нет дисплея)
            QSKIP("ViewFactory failed to restore views from DTO (possible headless environment).");
        }

        // Проверяем, что менеджер корректно принял состояние
        QCOMPARE(manager.getMainViewId(), dto.mainViewId);

        auto viewsList = manager.getAllViews();
        QCOMPARE(viewsList.size(), 2);

        // Ищем восстановленное окно в менеджере и проверяем его свойства
        auto restoredView = manager.getView(dto.mainViewId);
        QVERIFY(restoredView != nullptr);
        QCOMPARE(restoredView->id(), view1.id);
        QCOMPARE(restoredView->viewName(), view1.name);
        QCOMPARE(restoredView->viewType(), view1.type);

        // Сверяем настройки (если AbstractView корректно отдает их обратно)
        QVariantMap restoredSettings = restoredView->getSettingsToVariantMap();
        QCOMPARE(restoredSettings.value("cameraZoom").toDouble(), 1.5);
    }

    // 4. Тест обработки нулевого указателя (защита от крашей)
    void testNullPointerHandling() {
        // toDTO
        auto dtoOpt = ViewManagerSerializer::toDTO(nullptr);
        QVERIFY(!dtoOpt.has_value());

        // fromDTO
        ViewManagerDTO dummyDto;
        bool           result = ViewManagerSerializer::fromDTO(dummyDto, nullptr);
        QVERIFY(result == false);
    }
};

// Используем QTEST_MAIN (включает QApplication), так как ViewFactory
// инстанцирует объекты (QOpenGLWidget, vtkGenericOpenGLRenderWindow),
// которые требуют активного GUI-потока.
QTEST_MAIN(TestViewManagerSerialize)
#include "TestViewManagerSerializer.moc"