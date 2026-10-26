#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <vtkPolyData.h>
#include <memory>

#include "Common/Structures/ObjectRegistryStructures.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/SessionManager/SessionManager.h"
#include "Core/ViewManager/ViewManager.h"
#include "Common/Enums/ViewEnums.h"

using namespace QSpace::Core;

class TestSessionManager : public QObject {
    Q_OBJECT

  private slots:
    void init() {
        m_registry     = std::make_unique<ObjectRegistry>();
        m_layerManager = std::make_unique<LayerManager>();
        m_viewManager  = std::make_unique<ViewManager>();
        m_sessionManager =
            std::make_unique<SessionManager>(m_registry.get(), m_layerManager.get(), m_viewManager.get());
    }

    void testSaveAndLoadEmptyProject() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString filePath = dir.filePath("empty_project.json");

        QVERIFY(m_sessionManager->saveProject(filePath, QSpace::Session::ProjectFormat::Json));
        QVERIFY(QFile::exists(filePath));
        QVERIFY(m_sessionManager->loadProject(filePath));
        QCOMPARE(m_registry->nodeCount(), 0);
    }

    void testSaveAndLoadNodeMetadata() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString filePath = dir.filePath("project.json");

        auto node  = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Node1");
        node->path = "/data/file.vtk";
        m_registry->registerNode(node);

        QVERIFY(m_sessionManager->saveProject(filePath, QSpace::Session::ProjectFormat::Json));
        m_registry->clear();
        QVERIFY(m_sessionManager->loadProject(filePath));

        QCOMPARE(m_registry->nodeCount(), 1);
        const auto restoredNode = m_registry->getAllNodes().front();
        QCOMPARE(restoredNode->label, node->label);
        QCOMPARE(restoredNode->path, node->path);
    }

    void testPaletteRoundTrip() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString filePath = dir.filePath("palette.json");

        QSpace::Visualize::ColorMap palette;
        palette.id   = QUuid::createUuid();
        palette.name = "Test palette";
        palette.points.append({0.0, 0.0, 0.1, 0.2});
        palette.points.append({1.0, 0.9, 0.8, 0.7});

        QVERIFY(m_sessionManager->savePalette(palette, filePath));
        const auto restoredPalette = m_sessionManager->loadPalette(filePath);
        QVERIFY(restoredPalette.has_value());
        QCOMPARE(restoredPalette->id, palette.id);
        QCOMPARE(restoredPalette->name, palette.name);
        QCOMPARE(restoredPalette->points.size(), palette.points.size());
    }

    void testReportsErrorsForInvalidPaths() {
        QSignalSpy errors(m_sessionManager.get(), &SessionManager::errorOccurred);

        QVERIFY(!m_sessionManager->saveProject({}, QSpace::Session::ProjectFormat::Json));
        QVERIFY(!m_sessionManager->loadProject({}));
        QVERIFY(!m_sessionManager->savePalette({}, {}));
        QVERIFY(!m_sessionManager->loadPalette({}).has_value());
        QCOMPARE(errors.count(), 4);
    }

    void testSaveAndLoadExperimentSnapshotNodeHierarchy() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString filePath = dir.filePath("hierarchy.json");

        auto experiment = std::make_shared<Experiment>("Simulation");
        auto snapshot = std::make_shared<Snapshot>("Step 12", 12.5);
        auto node = std::make_shared<DataNode>(nullptr, "Gas", 12.5);
        node->path = "simulation/step12.vtk";
        node->format = QSpace::IO::FileFormat::Unknown;
        snapshot->addComponent(node);
        experiment->addSnapshot(snapshot);
        m_registry->registerExperiment(experiment);

        QVERIFY(m_sessionManager->saveProject(filePath, QSpace::Session::ProjectFormat::Json));
        m_registry->clear();
        QVERIFY(m_sessionManager->loadProject(filePath));

        QCOMPARE(m_registry->experimentCount(), 1);
        QCOMPARE(m_registry->snapshotCount(), 1);
        QCOMPARE(m_registry->nodeCount(), 1);
        const auto restoredExperiment = m_registry->getExperiment(experiment->id);
        QVERIFY(restoredExperiment);
        QCOMPARE(restoredExperiment->name, experiment->name);
        QCOMPARE(restoredExperiment->snapshots.size(), 1);
        const auto restoredSnapshot = restoredExperiment->snapshots.front();
        QCOMPARE(restoredSnapshot->id, snapshot->id);
        QCOMPARE(restoredSnapshot->timestamp, snapshot->timestamp);
        QCOMPARE(restoredSnapshot->components.size(), 1);
        const auto restoredNode = restoredSnapshot->components.front();
        QCOMPARE(restoredNode->id, node->id);
        QCOMPARE(restoredNode->label, node->label);
        QCOMPARE(restoredNode->path, node->path);
    }

    void testSaveAndLoadWindowsAndLayers() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString filePath = dir.filePath("window_layer_project.json");

        auto node = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Gas");
        m_registry->registerNode(node);
        const QUuid viewId = m_viewManager->createView(QSpace::Visualize::Views::ViewType::OpenGL3D);
        QVERIFY(!viewId.isNull());
        m_viewManager->setMainView(viewId);
        const QUuid layerId = m_layerManager->createLayer(node, m_viewManager->getView(viewId));
        QVERIFY(!layerId.isNull());

        QVERIFY(m_sessionManager->saveProject(filePath, QSpace::Session::ProjectFormat::Json));
        QVERIFY(m_sessionManager->loadProject(filePath));

        QCOMPARE(m_viewManager->getAllViews().size(), 1);
        QCOMPARE(m_viewManager->getMainViewId(), viewId);
        QCOMPARE(m_layerManager->getAllLayers().size(), 1);
        const auto restoredLayer = m_layerManager->getLayer(layerId);
        QVERIFY(restoredLayer);
        QCOMPARE(restoredLayer->dataNodeId(), node->id);
    }

    void testLoadProjectRejectsCorruptedJson() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString filePath = dir.filePath("corrupted.json");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QCOMPARE(file.write("{ invalid json"), qint64(14));
        file.close();

        QSignalSpy errors(m_sessionManager.get(), &SessionManager::errorOccurred);
        QVERIFY(!m_sessionManager->loadProject(filePath));
        QCOMPARE(errors.count(), 1);
    }

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
    std::unique_ptr<LayerManager>   m_layerManager;
    std::unique_ptr<ViewManager>    m_viewManager;
    std::unique_ptr<SessionManager> m_sessionManager;
};

QTEST_MAIN(TestSessionManager)
#include "TestSessionManager.moc"