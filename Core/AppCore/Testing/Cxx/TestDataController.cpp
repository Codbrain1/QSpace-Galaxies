#include <QDataStream>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "Common/Structures/ObjectRegistryStructures.h"
#include "Core/AppCore/DataController.h"
#include "Core/DataManager/DataManager.h"
#include "Core/FileSchemeRegistry/FileSchemeRegistry.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/TaskManager/TaskManager.h"

using namespace QSpace::Core;
using namespace QSpace::Core::Controllers;

class TestDataController : public QObject {
    Q_OBJECT

  private slots:
    void init() {
        m_taskManager        = std::make_unique<TaskManager>();
        m_dataManager        = std::make_unique<DataManager>(m_taskManager.get());
        m_registry           = std::make_unique<ObjectRegistry>();
        m_layerManager       = std::make_unique<LayerManager>();
        m_fileSchemeRegistry = std::make_unique<FileSchemeRegistry>();
        m_fileSchemeRegistry->registerStandartPresets();
        m_controller = std::make_unique<DataController>(m_dataManager.get(),
                                                        m_registry.get(),
                                                        m_layerManager.get(),
                                                        nullptr,
                                                        m_fileSchemeRegistry.get());
        m_controller->initialize();
    }

    void testFindsNodeByFilePath() {
        auto node  = std::make_shared<DataNode>(nullptr, "Imported data");
        node->path = "data.bin";
        m_registry->registerNode(node);

        QCOMPARE(m_controller->getNodeIdByFilePath(node->path), std::optional<QUuid>(node->id));
        QVERIFY(!m_controller->getNodeIdByFilePath("missing.bin").has_value());
        QCOMPARE(m_controller->getNodeById(node->id), node);
    }

    void testFindsExperimentContainingNodePath() {
        auto node  = std::make_shared<DataNode>(nullptr, "Snapshot node");
        node->path = "step_001.bin";
        auto snapshot = std::make_shared<Snapshot>("Step 1");
        snapshot->addComponent(node);
        auto experiment = std::make_shared<Experiment>("Experiment");
        experiment->addSnapshot(snapshot);
        m_registry->registerExperiment(experiment);
        m_registry->registerSnapshot(snapshot, experiment->id);
        m_registry->registerNodeToSnapshot(node, snapshot->id);

        QCOMPARE(m_controller->getExperimentIdByNodePath(node->path),
                 std::optional<QUuid>(experiment->id));
        QVERIFY(!m_controller->getExperimentIdByNodePath("missing.bin").has_value());
        QCOMPARE(m_controller->getExperiments().size(), 1);
    }

    void testCreatesExperimentFromValidRequest() {
        m_controller->importExperiment(QStringList{QStringLiteral("Gas.bin")},
                                       QStringLiteral("Imported experiment"));

        QCOMPARE(m_registry->experimentCount(), 1);
        QCOMPARE(m_controller->getExperiments().front()->name, QStringLiteral("Imported experiment"));
    }

    void testRejectsInvalidExperimentRequests() {
        m_controller->importExperiment({}, QStringLiteral("Experiment"));
        m_controller->importExperiment(QStringList{QStringLiteral("Gas.bin")}, {});

        QCOMPARE(m_registry->experimentCount(), 0);
    }

    void testImportsSingleBinFileAsynchronously() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = createGasBin(directory.path() + "/Gas_single.bin", 1.25);

        QSignalSpy dirtySpy(m_controller.get(), &DataController::markSessionDirty);
        QSignalSpy sceneSpy(m_controller.get(), &DataController::sceneUpdateRequested);
        m_controller->importFiles({path});

        QTRY_COMPARE(m_registry->nodeCount(), 1);
        QTRY_COMPARE(sceneSpy.count(), 1);
        QCOMPARE(dirtySpy.count(), 1);
        auto node = m_registry->getAllNodes().front();
        QCOMPARE(node->path, path);
        QCOMPARE(node->type, QSpace::Visualize::EntityType::Gas);
        QVERIFY(node->data != nullptr);
    }

    void testImportsBatchIntoGroupedExperimentSnapshots() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString first  = createGasBin(directory.path() + "/Gas_a.bin", 4.0);
        const QString second = createGasBin(directory.path() + "/Gas_b.bin", 4.0);

        m_controller->importExperiment({first, second}, QStringLiteral("Batch experiment"));

        QTRY_COMPARE(m_registry->nodeCount(), 2);
        QTRY_COMPARE(m_registry->snapshotCount(), 1);
        QCOMPARE(m_registry->experimentCount(), 1);
        const auto experiment = m_controller->getExperiments().front();
        QCOMPARE(experiment->name, QStringLiteral("Batch experiment"));
        QCOMPARE(experiment->snapshots.size(), 1);
        QCOMPARE(experiment->snapshots.front()->components.size(), 2);
    }

    void testRejectsFailedSingleImportWithoutRegisteringNode() {
        QSignalSpy finishedSpy(m_dataManager.get(), &DataManager::ioFinished);
        QSignalSpy sceneSpy(m_controller.get(), &DataController::sceneUpdateRequested);
        m_controller->importFiles({QStringLiteral("Gas_missing.bin")});

        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(finishedSpy.front().front().toBool(), false);
        QCOMPARE(m_registry->nodeCount(), 0);
        QCOMPARE(sceneSpy.count(), 0);
    }

    void testLazyLoadsNodeAndPreventsDuplicateRequests() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = createGasBin(directory.path() + "/Gas_lazy.bin", 2.5);
        auto node = std::make_shared<DataNode>(nullptr, "Lazy node");
        node->path   = path;
        node->format = QSpace::IO::FileFormat::BIN;
        node->type   = QSpace::Visualize::EntityType::Gas;
        m_registry->registerNode(node);

        QSignalSpy startedSpy(m_dataManager.get(), &DataManager::ioStarted);
        QSignalSpy loadedSpy(m_controller.get(), &DataController::nodeDataLoaded);
        m_controller->onRequestDataLoad(node->id);
        m_controller->onRequestDataLoad(node->id);

        QCOMPARE(startedSpy.count(), 1);
        QTRY_COMPARE(loadedSpy.count(), 1);
        QCOMPARE(loadedSpy.front().front().value<QUuid>(), node->id);
        QVERIFY(node->data != nullptr);
    }

    void testLazyLoadFailureReleasesRequestLock() {
        auto node = std::make_shared<DataNode>(nullptr, "Missing node");
        node->path   = QStringLiteral("Gas_missing.bin");
        node->format = QSpace::IO::FileFormat::BIN;
        node->type   = QSpace::Visualize::EntityType::Gas;
        m_registry->registerNode(node);

        QSignalSpy startedSpy(m_dataManager.get(), &DataManager::ioStarted);
        m_controller->onRequestDataLoad(node->id);
        QCOMPARE(startedSpy.count(), 1);
        const QUuid taskId = startedSpy.front().front().value<QUuid>();

        QSpace::IO::ReadResult failedResult;
        failedResult.path   = node->path;
        failedResult.status = QSpace::IO::ReadStatus::FileNotFound;
        failedResult.errMessage = QStringLiteral("not found");
        emit m_dataManager->fileReady(taskId, failedResult, QSpace::IO::ImportRole::ProjectData);

        m_controller->onRequestDataLoad(node->id);
        QCOMPARE(startedSpy.count(), 2);
        QVERIFY(node->data == nullptr);
    }

    void testRemovesNodeAndMarksSessionDirty() {
        auto node = std::make_shared<DataNode>(nullptr, "Node to remove");
        m_registry->registerNode(node);
        QSignalSpy dirtySpy(m_controller.get(), &DataController::markSessionDirty);

        m_controller->removeNodeObject(node->id);

        QVERIFY(!m_registry->getNode(node->id));
        QCOMPARE(dirtySpy.count(), 1);
    }

  private:
    static QString createGasBin(const QString& path, double timestamp) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};

        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << qint32(1) << timestamp;
        for (int index = 0; index < 9; ++index)
            stream << double(index);
        stream << qint32(0);
        return path;
    }

    std::unique_ptr<TaskManager>        m_taskManager;
    std::unique_ptr<DataManager>        m_dataManager;
    std::unique_ptr<ObjectRegistry>     m_registry;
    std::unique_ptr<LayerManager>       m_layerManager;
    std::unique_ptr<FileSchemeRegistry> m_fileSchemeRegistry;
    std::unique_ptr<DataController>     m_controller;
};

QTEST_GUILESS_MAIN(TestDataController)
#include "TestDataController.moc"
