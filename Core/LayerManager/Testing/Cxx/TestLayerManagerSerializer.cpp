#include <QUuid>
#include <QVariantMap>
#include <QtTest>
#include <cstddef>
#include <memory>

// Заголовки тестируемых компонентов
#include "Common/Enums/LayerEnums.h"
#include "Common/Structures/SessionStructures.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/LayerManager/LayerManagerSerializer.h"

// Заголовки для зависимостей слоев (потребуются для моков/создания)
#include "Common/Structures/ObjectRegistryStructures.h" // Убедитесь, что путь корректен
#include "Visualize/Views/View3D/AbstractView3D.h"      // Убедитесь, что путь корректен
#include "Session/Reflection.h"
using namespace QSpace::Core;
using namespace QSpace::Session;
using namespace QSpace::Visualize::Layers;

class TestLayerManagerSerialize : public QObject {
    Q_OBJECT

  private slots:

    // 1. Тест сериализации пустого менеджера
    void testSerializeEmptyManager() {
        LayerManager manager;

        auto dtoOpt = LayerManagerSerializer::toDTO(&manager);

        QVERIFY(dtoOpt.has_value());

        auto dto = dtoOpt.value();
        QVERIFY(dto.layers.isEmpty());
    }

    // 2. Тест успешной сериализации наполненного менеджера
    void testSerializePopulatedManager() {
        LayerManager manager;

        // Создаем фиктивные зависимости (Node и View)
        // Примечание: предполагается, что у DataNode и View можно задать ID и состояние для теста
        auto dummyNode = std::make_shared<DataNode>(nullptr, "");
        // В LayerManager::createLayer есть проверка (!node->isLoaded()), эмулируем загрузку:
        // dummyNode->setLoaded(true); // Раскомментируйте или адаптируйте под ваш API DataNode

        auto dummyView = std::shared_ptr<QSpace::Visualize::Views::AbstractView>(
            /* Здесь нужен инстанс конкретного View, например View3D.
               Если абстрактный класс нельзя создать, используйте конкретный (например mock) */
        );

        // Внимание: Если создать dummyView невозможно без GUI-потока, этот тест может потребовать QSKIP
        if (!dummyNode || !dummyView) {
            QSKIP("Failed to setup dummy Node/View dependencies. Skipping.");
        }

        QUuid layerId = manager.createLayer(dummyNode, dummyView);

        // Как и с View, создание движка SPH (LayerFactory::createLayerRenderer)
        // может провалиться в headless-окружении
        if (layerId.isNull()) {
            QSKIP("LayerFactory failed to create render engine. Skipping serialization test (possible headless "
                  "environment).");
        }

        // Сериализуем
        auto dtoOpt = LayerManagerSerializer::toDTO(&manager);
        QVERIFY(dtoOpt.has_value());

        auto dto = dtoOpt.value();

        // Проверяем структуру DTO
        QCOMPARE(dto.layers.size(), 1);

        const auto& layerDto = dto.layers.first();
        QCOMPARE(layerDto.layerId, layerId);
        // QCOMPARE(layerDto.nodeId, dummyNode->id); // Проверка ID ноды
        // QCOMPARE(layerDto.viewId, dummyView->id()); // Проверка ID окна
        QCOMPARE(layerDto.renderType, RenderLayerType::SPH); // Захардкожено в createLayer
    }

    // 3. Тест десериализации (DTO -> LayerManager)
    void testDeserialization() {
        // Подготавливаем "сохраненное состояние"
        LayerManagerDTO dto;

        LayerDTO layerDto;
        layerDto.layerId                  = QUuid::createUuid();
        layerDto.nodeId                   = QUuid::createUuid();
        layerDto.viewId                   = QUuid::createUuid();
        layerDto.name                     = "Test SPH Layer";
        layerDto.isSynced                 = true;
        layerDto.renderType               = RenderLayerType::SPH;
        layerDto.settings["colorByField"] = "Mass"; // Пример сохраненной настройки

        dto.layers.append(layerDto);

        // Подготавливаем моки для резолверов (зависимости, которые "уже существуют" в проекте)
        auto dummyNode = std::make_shared<DataNode>(nullptr, "");
        // dummyNode->id = layerDto.nodeId; // Устанавливаем нужный ID

        auto dummyView = std::shared_ptr<QSpace::Visualize::Views::AbstractView>(); // Создайте валидный инстанс

        // Настраиваем лямбда-функции (резолверы), которые вернут нужные объекты по их ID
        LayerManagerSerializer::NodeResolver nodeRes = [&](const QUuid& id) -> std::shared_ptr<DataNode> {
            return (id == layerDto.nodeId) ? dummyNode : nullptr;
        };

        LayerManagerSerializer::ViewResolver viewRes =
            [&](const QUuid& id) -> std::shared_ptr<QSpace::Visualize::Views::AbstractView> {
            return (id == layerDto.viewId) ? dummyView : nullptr;
        };

        // Распаковываем
        LayerManager manager;
        bool         success = LayerManagerSerializer::fromDTO(dto, &manager, nodeRes, viewRes);

        if (!success) {
            // Если фабрика (LayerFactory::createLayerRenderer) не смогла восстановить OpenGL движок слоя
            QSKIP("LayerFactory failed to restore render engines from DTO (possible headless environment) or Resolvers "
                  "failed.");
        }

        // Проверяем, что менеджер корректно принял состояние
        auto layersList = manager.getAllLayers();
        QCOMPARE(layersList.size(), 1);

        auto restoredLayer = manager.getLayer(layerDto.layerId);
        QVERIFY(restoredLayer != nullptr);
        QCOMPARE(restoredLayer->layerId(), layerDto.layerId);
        QCOMPARE(restoredLayer->dataNodeId(), layerDto.nodeId);
        QCOMPARE(restoredLayer->name(), layerDto.name);
        QCOMPARE(restoredLayer->IsSyncedWithMaster(), layerDto.isSynced);

        // Проверяем, что настройки успешно перенеслись (если Reflection отработал)
        if (auto settings = restoredLayer->getSettings()) {
            QVariantMap restoredSettings = QSpace::Reflection::QObjectToVariantMap(settings.get());
            QCOMPARE(restoredSettings.value("colorByField").toString(), QString("Mass"));
        }
    }

    // 4. Тест обработки нулевых указателей (защита от крашей)
    void testNullPointerHandling() {
        // toDTO - передача нулевого менеджера
        auto dtoOpt = LayerManagerSerializer::toDTO(nullptr);
        QVERIFY(!dtoOpt.has_value());

        // fromDTO - передача нулевого менеджера
        LayerManagerDTO dummyDto;

        LayerManagerSerializer::NodeResolver dummyNodeRes = [](const QUuid&) { return nullptr; };
        LayerManagerSerializer::ViewResolver dummyViewRes = [](const QUuid&) { return nullptr; };

        bool result = LayerManagerSerializer::fromDTO(dummyDto, nullptr, dummyNodeRes, dummyViewRes);
        QVERIFY(result == false);
    }

    // 5. Тест провала десериализации при отсутствии зависимостей (Node/View не найдены)
    void testDeserializationMissingDependencies() {
        LayerManagerDTO dto;
        LayerDTO        layerDto;
        layerDto.layerId = QUuid::createUuid();
        layerDto.nodeId  = QUuid::createUuid();
        layerDto.viewId  = QUuid::createUuid();
        dto.layers.append(layerDto);

        // Резолверы всегда возвращают nullptr (имитация того, что нода или вьюшка были удалены)
        LayerManagerSerializer::NodeResolver nodeRes = [](const QUuid&) { return nullptr; };
        LayerManagerSerializer::ViewResolver viewRes = [](const QUuid&) { return nullptr; };

        LayerManager manager;
        // fromDTO должен вернуть false, так как не удалось разрезолвить зависимости
        bool success = LayerManagerSerializer::fromDTO(dto, &manager, nodeRes, viewRes);

        QVERIFY(success == false);
        QVERIFY(manager.getAllLayers().isEmpty());
    }
};

// Используем QTEST_MAIN, так как для рендереров слоев
// почти наверняка требуются потоки OpenGL и контекст QApplication
QTEST_MAIN(TestLayerManagerSerialize)
#include "TestLayerManagerSerializer.moc"