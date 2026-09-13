#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/ObjectRegistry/ObjectRegistrySerializer.h"
#include <QtTest>
#include <qtestcase.h>
#include <memory>

class TestObjectRegistrySerializer : public QObject {
    Q_OBJECT
    std::unique_ptr<QSpace::Core::ObjectRegistry> objectRegistry;
    void                                          init();
    void                                          initSeveralObjects();
  private slots:

    void cleanup();
    void initTestCase();
    void testSerializeObjectRegistry();
    void testSerializeObjectRegistrySeveralObjects();
    void testDeserializeObjectRegistry();
    void testDeserializeObjectRegistrySeveralObjects();
};

void TestObjectRegistrySerializer::init() {
    objectRegistry = std::make_unique<QSpace::Core::ObjectRegistry>();
    auto dataNode =
        std::make_shared<QSpace::Core::DataNode>(nullptr, "TestDataNode", 1.0, QSpace::Visualize::EntityType::Stars);
    dataNode->path            = "D:/Test/F";
    dataNode->scheme          = QUuid::createUuid();
    dataNode->format          = QSpace::IO::FileFormat::BIN;
    dataNode->stats.bounds[0] = 1;
    dataNode->stats.bounds[1] = 1;
    dataNode->stats.bounds[2] = 1;
    dataNode->stats.bounds[3] = -1;
    dataNode->stats.bounds[4] = -1;
    dataNode->stats.bounds[5] = -1;
    dataNode->stats.cellCount = 10;
    dataNode->stats.center[0] = 2;
    dataNode->stats.center[0] = 3;
    dataNode->stats.center[0] = 4;


    auto snapshot = std::make_shared<QSpace::Core::Snapshot>("TestSnapshot", 1.0);
    snapshot->addComponent(dataNode);
    auto experiment = std::make_shared<QSpace::Core::Experiment>("TestExperiment");
    experiment->addSnapshot(snapshot);
    objectRegistry->registerExperiment(experiment);
}

void TestObjectRegistrySerializer::initSeveralObjects() {
    objectRegistry     = std::make_unique<QSpace::Core::ObjectRegistry>();
    auto getEntityType = [](int i) {
        if (i == 0) {
            return QSpace::Visualize::EntityType::Stars;
        } else if (i == 1) {
            return QSpace::Visualize::EntityType::DarkMatter;
        } else if (i == 2) {
            return QSpace::Visualize::EntityType::Gas;
        } else if (i == 3) {
            return QSpace::Visualize::EntityType::Mixed;
        }
        return QSpace::Visualize::EntityType::Unknown;
    };

    auto getFileFormat = [](int i) {
        if (i == 0) {
            return QSpace::IO::FileFormat::BIN;
        } else if (i == 1) {
            return QSpace::IO::FileFormat::GRD;
        } else if (i == 2) {
            return QSpace::IO::FileFormat::HDF5;
        } else if (i == 3) {
            return QSpace::IO::FileFormat::TXT;
        }
        return QSpace::IO::FileFormat::Unknown;
    };


    for (int k = 0; k < 2; ++k) {
        auto experiment = std::make_shared<QSpace::Core::Experiment>(QString("TestExperiment%1").arg(k));

        for (int j = 0; j < 3; ++j) {
            auto snapshot =
                std::make_shared<QSpace::Core::Snapshot>(experiment->name + QString("TestSnapshot%1").arg(j), j);

            for (int i = 0; i < 5; ++i) {
                auto dataNode =
                    std::make_shared<QSpace::Core::DataNode>(nullptr,
                                                             snapshot->name + QString("TestDataNode%1").arg(i),
                                                             i,
                                                             getEntityType(i));
                dataNode->path   = QString("D:/Test/F%1").arg(i);
                dataNode->scheme = QUuid::createUuid();
                dataNode->format = getFileFormat(i);
                snapshot->addComponent(dataNode);
            }
            experiment->addSnapshot(snapshot);
        }
        objectRegistry->registerExperiment(experiment);
    }
}

void TestObjectRegistrySerializer::cleanup() {
    if (objectRegistry) {
        objectRegistry->clear();
    }
}

void TestObjectRegistrySerializer::initTestCase() {
}

void TestObjectRegistrySerializer::testSerializeObjectRegistry() {
    init();
    auto dto_ptr = QSpace::Core::ObjectRegistrySerializer::toDTO(objectRegistry.get());
    QVERIFY(dto_ptr.has_value());
    auto& dto = dto_ptr.value();

    // проверяем что записю в dto вообще произошла
    QVERIFY(!dto.experiments.isEmpty());
    QVERIFY(!dto.snapshots.isEmpty());
    QVERIFY(!dto.nodes.isEmpty());

    // сравниваем эксперимент
    auto experiment = objectRegistry->getAllExperiments().first();
    QCOMPARE(dto.experiments.first().id, experiment->id);
    QCOMPARE(dto.experiments.first().name, experiment->name);

    QVERIFY(!experiment->snapshots.isEmpty());
    QCOMPARE(experiment->snapshots.size(), 1);

    // сравниваем снапшот и проверяем что он добавлен в dto
    auto snapshot = experiment->snapshots.first();
    QCOMPARE(snapshot->id, dto.snapshots.first().id);
    QCOMPARE(snapshot->name, dto.snapshots.first().name);

    QVERIFY(!snapshot->components.isEmpty());
    QCOMPARE(snapshot->components.size(), 1);

    auto node = snapshot->components.first();
    QCOMPARE(node->id, dto.nodes.first().id);
    QCOMPARE(node->label, dto.nodes.first().name);
    QCOMPARE(node->path, dto.nodes.first().path);
    QCOMPARE(node->type, dto.nodes.first().type);
    QCOMPARE(node->stats, dto.nodes.first().stats);
    QCOMPARE(node->format, dto.nodes.first().format);
    QCOMPARE(node->scheme, dto.nodes.first().scheme);
}

void TestObjectRegistrySerializer::testSerializeObjectRegistrySeveralObjects() {
    // Инициализируем реестр сложной иерархией объектов
    initSeveralObjects();

    // Сериализуем
    auto dto_ptr = QSpace::Core::ObjectRegistrySerializer::toDTO(objectRegistry.get());

    QVERIFY(dto_ptr.has_value());
    auto& dto = dto_ptr.value();

    // Проверяем общее количество объектов в DTO
    // Ожидаем: 2 эксперимента, 6 снапшотов, 30 нод
    QCOMPARE(dto.experiments.size(), 2);
    QCOMPARE(dto.snapshots.size(), 6);
    QCOMPARE(dto.nodes.size(), 30);

    // Создаем быструю хеш-таблицу DTO для проверки связей по UUID
    QHash<QUuid, QSpace::Session::SnapshotDTO> dtoSnapshotsMap;
    for (const auto& snapDto : dto.snapshots) {
        dtoSnapshotsMap.insert(snapDto.id, snapDto);
    }

    QHash<QUuid, QSpace::Session::DataNodeDTO> dtoNodesMap;
    for (const auto& nodeDto : dto.nodes) {
        dtoNodesMap.insert(nodeDto.id, nodeDto);
    }

    // Проверяем корректность связей для каждого эксперимента из оригинального реестра
    auto origExperiments = objectRegistry->getAllExperiments();
    for (const auto& origExp : origExperiments) {
        // Находим соответствующий ExperimentDTO
        auto expDtoIt =
            std::find_if(dto.experiments.begin(),
                         dto.experiments.end(),
                         [&origExp](const QSpace::Session::ExperimentDTO& item) { return item.id == origExp->id; });

        QVERIFY2(expDtoIt != dto.experiments.end(), "ExperimentDTO for original experiment not found!");
        QCOMPARE(expDtoIt->name, origExp->name);
        QCOMPARE(expDtoIt->snapshots.size(), origExp->snapshots.size());

        // Проверяем входящие в эксперимент снапшоты
        for (const auto& origSnap : origExp->snapshots) {
            // Проверяем, что UUID снапшота записан в списки эксперимента
            QVERIFY(expDtoIt->snapshots.contains(origSnap->id));

            // Проверяем сам SnapshotDTO
            QVERIFY(dtoSnapshotsMap.contains(origSnap->id));
            const auto& snapDto = dtoSnapshotsMap.value(origSnap->id);

            QCOMPARE(snapDto.name, origSnap->name);
            QCOMPARE(snapDto.nodes.size(), origSnap->components.size());

            // Проверяем входящие в снапшот ноды
            for (const auto& origNode : origSnap->components) {
                // Проверяем, что UUID ноды записан в списки снапшота
                QVERIFY(snapDto.nodes.contains(origNode->id));

                // Проверяем сам DataNodeDTO
                QVERIFY(dtoNodesMap.contains(origNode->id));
                const auto& nodeDto = dtoNodesMap.value(origNode->id);

                QCOMPARE(nodeDto.name, origNode->label);
                QCOMPARE(nodeDto.path, origNode->path);
                QCOMPARE(nodeDto.type, origNode->type);
                QCOMPARE(nodeDto.format, origNode->format);
                QCOMPARE(nodeDto.scheme, origNode->scheme);
                QCOMPARE(nodeDto.stats, origNode->stats);
            }
        }
    }
}

void TestObjectRegistrySerializer::testDeserializeObjectRegistry() {
    // Инициализируем исходные данные и формируем DTO
    init();
    auto dto_ptr = QSpace::Core::ObjectRegistrySerializer::toDTO(objectRegistry.get());

    QVERIFY(dto_ptr.has_value());
    auto& dto = dto_ptr.value();

    // Создаем совершенно новый чистый реестр для восстановления
    auto restoredRegistry = std::make_unique<QSpace::Core::ObjectRegistry>();

    // Вызываем десериализацию и проверяем успешность возврата
    bool success = QSpace::Core::ObjectRegistrySerializer::fromDTO(dto, restoredRegistry.get());
    QVERIFY(success);

    // Проверяем, что в восстановленном реестре созданы объекты
    auto experiments = restoredRegistry->getAllExperiments();
    QVERIFY(!experiments.isEmpty());
    QCOMPARE(experiments.size(), 1);

    // Проверяем восстановленный эксперимент
    auto experiment = experiments.first();
    QCOMPARE(experiment->id, dto.experiments.first().id);
    QCOMPARE(experiment->name, dto.experiments.first().name);

    QVERIFY(!experiment->snapshots.isEmpty());
    QCOMPARE(experiment->snapshots.size(), 1);

    // Проверяем восстановленный снапшот и связь с экспериментом
    auto snapshot = experiment->snapshots.first();
    QCOMPARE(snapshot->id, dto.snapshots.first().id);
    QCOMPARE(snapshot->name, dto.snapshots.first().name);

    QVERIFY(!snapshot->components.isEmpty());
    QCOMPARE(snapshot->components.size(), 1);

    // Проверяем восстановленную ноду, ее поля и связь со снапшотом
    auto node = snapshot->components.first();
    QCOMPARE(node->id, dto.nodes.first().id);
    QCOMPARE(node->label, dto.nodes.first().name);
    QCOMPARE(node->path, dto.nodes.first().path);
    QCOMPARE(node->type, dto.nodes.first().type);
    QCOMPARE(node->stats, dto.nodes.first().stats);
    QCOMPARE(node->format, dto.nodes.first().format);
    QCOMPARE(node->scheme, dto.nodes.first().scheme);
}

void TestObjectRegistrySerializer::testDeserializeObjectRegistrySeveralObjects() {
    // Создаем исходные данные и получаем DTO с помощью сериализатора
    initSeveralObjects();
    auto dto_ptr = QSpace::Core::ObjectRegistrySerializer::toDTO(objectRegistry.get());
    QVERIFY(dto_ptr.has_value());
    auto& dto = dto_ptr.value();

    // Создаем совершенно новый чистый реестр для восстановления
    auto restoredRegistry = std::make_unique<QSpace::Core::ObjectRegistry>();

    // Вызываем десериализацию
    bool success = QSpace::Core::ObjectRegistrySerializer::fromDTO(dto, restoredRegistry.get());

    // Проверяем успешность выполнения
    QVERIFY2(success, "fromDTO returned false during deserialization!");

    // Проверяем общее количество восстановленных объектов в реестре
    auto restoredExperiments = restoredRegistry->getAllExperiments();
    QCOMPARE(restoredExperiments.size(), 2);

    // Рекурсивно проверяем целостность восстановленного графа объектов
    for (int k = 0; k < 2; ++k) {
        QString expectedExpName = QString("TestExperiment%1").arg(k);

        // Поиск эксперимента по имени
        auto expIt = std::find_if(restoredExperiments.cbegin(),
                                  restoredExperiments.cend(),
                                  [&expectedExpName](const std::shared_ptr<QSpace::Core::Experiment>& exp) {
                                      return exp && exp->name == expectedExpName;
                                  });

        QVERIFY2(expIt != restoredExperiments.cend(),
                 qPrintable(QString("Experiment '%1' was not found in restored registry").arg(expectedExpName)));

        auto exp = *expIt;
        QCOMPARE(exp->snapshots.size(), 3);

        // Проверяем восстановление снапшотов
        for (int j = 0; j < 3; ++j) {
            QString expectedSnapName = exp->name + QString("TestSnapshot%1").arg(j);

            auto snapIt = std::find_if(exp->snapshots.cbegin(),
                                       exp->snapshots.cend(),
                                       [&expectedSnapName](const std::shared_ptr<QSpace::Core::Snapshot>& snap) {
                                           return snap && snap->name == expectedSnapName;
                                       });

            QVERIFY2(
                snapIt != exp->snapshots.cend(),
                qPrintable(QString("Snapshot '%1' was not found in Experiment '%2'").arg(expectedSnapName, exp->name)));

            auto snap = *snapIt;
            QCOMPARE(snap->timestamp, static_cast<double>(j));
            QCOMPARE(snap->components.size(), 5);

            // Проверяем восстановление DataNode
            for (int i = 0; i < 5; ++i) {
                QString expectedNodeName = snap->name + QString("TestDataNode%1").arg(i);

                auto nodeIt = std::find_if(snap->components.cbegin(),
                                           snap->components.cend(),
                                           [&expectedNodeName](const std::shared_ptr<QSpace::Core::DataNode>& node) {
                                               return node && node->label == expectedNodeName;
                                           });

                QVERIFY2(
                    nodeIt != snap->components.cend(),
                    qPrintable(
                        QString("DataNode '%1' was not found in Snapshot '%2'").arg(expectedNodeName, snap->name)));

                auto node = *nodeIt;

                // Проверяем сохранность всех полей и метаданных ноды
                // QCOMPARE(node->stats, static_cast<double>(i));
                QCOMPARE(node->path, QString("D:/Test/F%1").arg(i));
                QVERIFY(!node->scheme.isNull());
            }
        }
    }
}

QTEST_MAIN(TestObjectRegistrySerializer)
#include "TestObjectRegistrySerialization.moc"