#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "Common/Structures/ObjectRegistryStructures.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/SessionManager/SessionManager.h"
#include "Core/ViewManager/ViewManager.h"
#include "Structures/SessionStructures.h"

using namespace QSpace::Core;
using namespace QSpace::Session;
using namespace QSpace::Visualize::Views;

class TestSessionManager : public QObject {
    Q_OBJECT

  private slots:
    void init() {
        m_registry       = std::make_unique<ObjectRegistry>();
        m_layerManager   = std::make_unique<LayerManager>();
        m_viewManager    = std::make_unique<ViewManager>();
        m_sessionManager =
            std::make_unique<SessionManager>(m_registry.get(), m_layerManager.get(), m_viewManager.get());
    }

    void testSaveAndLoadProjectDTO() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath("project.json");

        const QUuid viewId = m_viewManager->createView(ViewType::QCustonPlot2D);
        if (viewId.isNull()) {
            QSKIP("ViewFactory could not create a view in this environment");
        }

        auto node = std::make_shared<DataNode>(nullptr, "Node1", 12.5, QSpace::Visualize::EntityType::Stars);
        node->path   = "/data/file.bin";
        node->format = QSpace::IO::FileFormat::BIN;
        node->scheme = QUuid::createUuid();

        auto snapshot = std::make_shared<Snapshot>("Snapshot1", 12.5);
        snapshot->addComponent(node);
        auto experiment = std::make_shared<Experiment>("Experiment1");
        experiment->addSnapshot(snapshot);
        m_registry->registerExperiment(experiment);

        QVERIFY(m_sessionManager->saveProject(filePath, ProjectFormat::Json));
        QVERIFY(QFile::exists(filePath));
        QVERIFY(QFileInfo(filePath).size() > 0);

        m_registry->clear();
        m_viewManager->clear();

        QVERIFY(m_sessionManager->loadProject(filePath));

        QCOMPARE(m_registry->getAllExperiments().size(), 1);
        QCOMPARE(m_registry->getAllSnapshots().size(), 1);
        QCOMPARE(m_registry->getAllNodes().size(), 1);

        const auto restoredExperiment = m_registry->getAllExperiments().first();
        QCOMPARE(restoredExperiment->id, experiment->id);
        QCOMPARE(restoredExperiment->name, experiment->name);
        QCOMPARE(restoredExperiment->snapshots.size(), 1);

        const auto restoredSnapshot = restoredExperiment->snapshots.first();
        QCOMPARE(restoredSnapshot->id, snapshot->id);
        QCOMPARE(restoredSnapshot->name, snapshot->name);
        QCOMPARE(restoredSnapshot->timestamp, snapshot->timestamp);
        QCOMPARE(restoredSnapshot->components.size(), 1);

        const auto restoredNode = restoredSnapshot->components.first();
        QCOMPARE(restoredNode->id, node->id);
        QCOMPARE(restoredNode->label, node->label);
        QCOMPARE(restoredNode->path, node->path);
        QCOMPARE(restoredNode->format, node->format);
        QCOMPARE(restoredNode->scheme, node->scheme);

        QCOMPARE(m_viewManager->getAllViews().size(), 1);
        QCOMPARE(m_viewManager->getMainViewId(), viewId);
        const auto restoredView = m_viewManager->getView(viewId);
        QVERIFY(restoredView);
        QCOMPARE(restoredView->viewType(), ViewType::QCustonPlot2D);
    }

    void testSaveFailsForEmptyPath() {
        QSignalSpy errors(m_sessionManager.get(), &SessionManager::errorOccurred);

        QVERIFY(!m_sessionManager->saveProject({}, ProjectFormat::Json));
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.first().first().toString(), QString("Project file path is empty"));
    }

    void testLoadFailsForMissingFile() {
        QSignalSpy errors(m_sessionManager.get(), &SessionManager::errorOccurred);

        QVERIFY(!m_sessionManager->loadProject("missing-project.json"));
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.first().first().toString(), QString("Failed to load project file"));
    }

    void testSaveFailsForUnsupportedFormat() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QSignalSpy errors(m_sessionManager.get(), &SessionManager::errorOccurred);

        QVERIFY(!m_sessionManager->saveProject(directory.filePath("project.xml"), ProjectFormat::Xml));
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.first().first().toString(), QString("Unsupported project format"));
    }

    void testSaveAndLoadEmptyProject() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("empty.json");
        QSignalSpy loaded(m_sessionManager.get(), &SessionManager::projectLoaded);

        QVERIFY(m_sessionManager->saveProject(path, ProjectFormat::Json));
        QVERIFY(m_sessionManager->loadProject(path));
        QCOMPARE(loaded.count(), 1);
        QVERIFY(m_registry->getAllExperiments().isEmpty());
        QVERIFY(m_registry->getAllNodes().isEmpty());
        QVERIFY(m_layerManager->getAllLayers().isEmpty());
        QVERIFY(m_viewManager->getAllViews().isEmpty());
    }

    void testSaveProjectWithoutViewOrLayerManager() {
        ObjectRegistry registry;
        SessionManager manager(&registry, nullptr, nullptr);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        QVERIFY(manager.saveProject(directory.filePath("registry-only.json"), ProjectFormat::Json));
        QVERIFY(QFileInfo::exists(directory.filePath("registry-only.json")));
    }

    void testRejectsCorruptedJson() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("corrupted.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{not valid json");
        file.close();

        QSignalSpy errors(m_sessionManager.get(), &SessionManager::errorOccurred);
        QVERIFY(!m_sessionManager->loadProject(path));
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.first().first().toString(), QString("Failed to decode project file"));
    }

    void testRejectsJsonWithMissingDtoFields() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("missing-field.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{}\n");
        file.close();

        QVERIFY(!m_sessionManager->loadProject(path));
        QCOMPARE(m_registry->getAllExperiments().size(), 0);
        QCOMPARE(m_layerManager->getAllLayers().size(), 0);
    }

    void testRestoreFailureDoesNotMutateExistingState() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("invalid-reference.json");

        auto node = std::make_shared<DataNode>(nullptr, "ExistingNode");
        auto snapshot = std::make_shared<Snapshot>("ExistingSnapshot", 1.0);
        snapshot->addComponent(node);
        auto experiment = std::make_shared<Experiment>("ExistingExperiment");
        experiment->addSnapshot(snapshot);
        m_registry->registerExperiment(experiment);

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{\"objregDTO\":{},\"viewmanDTO\":{},\"layermanDTO\":{\"layers\":[{\"nodeId\":\"{00000000-0000-0000-0000-000000000001}\",\"viewId\":\"{00000000-0000-0000-0000-000000000002}\"}]},\"colormapmanDTO\":{}}");
        file.close();

        QVERIFY(!m_sessionManager->loadProject(path));
        QCOMPARE(m_registry->getAllExperiments().size(), 1);
        QCOMPARE(m_registry->getAllNodes().size(), 1);
        QCOMPARE(m_registry->getAllNodes().first()->label, QString("ExistingNode"));
    }

    void testMultipleObjectsRoundTrip() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("multiple.json");
        for (int experimentIndex = 0; experimentIndex < 2; ++experimentIndex) {
            auto experiment = std::make_shared<Experiment>(QString("Experiment%1").arg(experimentIndex));
            for (int snapshotIndex = 0; snapshotIndex < 2; ++snapshotIndex) {
                auto snapshot = std::make_shared<Snapshot>(QString("Snapshot%1").arg(snapshotIndex), snapshotIndex);
                for (int nodeIndex = 0; nodeIndex < 2; ++nodeIndex)
                    snapshot->addComponent(std::make_shared<DataNode>(nullptr, QString("Node%1").arg(nodeIndex)));
                experiment->addSnapshot(snapshot);
            }
            m_registry->registerExperiment(experiment);
        }

        QVERIFY(m_sessionManager->saveProject(path, ProjectFormat::Json));
        m_registry->clear();
        QVERIFY(m_sessionManager->loadProject(path));
        QCOMPARE(m_registry->getAllExperiments().size(), 2);
        QCOMPARE(m_registry->getAllSnapshots().size(), 4);
        QCOMPARE(m_registry->getAllNodes().size(), 8);
    }

    void testSaveLoadSaveProducesEquivalentJson() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString first = directory.filePath("first.json");
        const QString second = directory.filePath("second.json");
        m_registry->registerExperiment(std::make_shared<Experiment>("RoundTrip"));
        QVERIFY(m_sessionManager->saveProject(first, ProjectFormat::Json));
        QVERIFY(m_sessionManager->loadProject(first));
        QVERIFY(m_sessionManager->saveProject(second, ProjectFormat::Json));

        QFile firstFile(first), secondFile(second);
        QVERIFY(firstFile.open(QIODevice::ReadOnly));
        QVERIFY(secondFile.open(QIODevice::ReadOnly));
        QCOMPARE(firstFile.readAll(), secondFile.readAll());
    }

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
    std::unique_ptr<LayerManager>   m_layerManager;
    std::unique_ptr<ViewManager>    m_viewManager;
    std::unique_ptr<SessionManager> m_sessionManager;
};

QTEST_MAIN(TestSessionManager)
#include "TestSessionManager.moc"