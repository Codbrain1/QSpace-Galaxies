#include <QSignalSpy>
#include <QtTest>
#include <vtkPolyData.h>

#include "Core/AppCore/VideoController.h"
#include "Core/AppCore/ViewController.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/ViewManager/ViewManager.h"

using namespace QSpace::Core;
using namespace QSpace::Core::Controllers;
using QSpace::Visualize::Views::ViewType;

class TestVideoViewController : public QObject {
    Q_OBJECT
  private slots:
    void init() {
        registry = std::make_unique<ObjectRegistry>();
        layers = std::make_unique<LayerManager>();
        views = std::make_unique<ViewManager>();
        viewController = std::make_unique<ViewController>(views.get(), layers.get(), registry.get());
        viewController->initialize();
        video = std::make_unique<VideoController>(registry.get(), views.get(), layers.get());
        video->initialize();
    }

    void testViewAndLayerLifecycle() {
        auto node = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Node");
        registry->registerNode(node);
        QSignalSpy created(viewController.get(), &ViewController::viewCreated);
        QSignalSpy removed(viewController.get(), &ViewController::viewRemoved);
        QSignalSpy layerCreated(layers.get(), &LayerManager::layerCreated);
        QSignalSpy layerRemoved(layers.get(), &LayerManager::layerRemoved);
        const QUuid first = viewController->createView(ViewType::OpenGL3D);
        const QUuid second = viewController->createView(ViewType::OpenGL3D);
        QVERIFY(!first.isNull());
        QVERIFY(!second.isNull());
        QCOMPARE(created.count(), 2);
        QCOMPARE(layerCreated.count(), 2);
        QCOMPARE(layers->getLayersForNode(node->id).size(), 2);
        layers->setNodeVisibility(node->id, false);
        for (const auto& layer : layers->getLayersForNode(node->id)) QVERIFY(!layer->getSettings()->isVisible());
        layers->setNodeVisibility(node->id, true);
        for (const auto& layer : layers->getLayersForNode(node->id)) QVERIFY(layer->getSettings()->isVisible());
        viewController->removeView(first);
        QCOMPARE(removed.count(), 1);
        QCOMPARE(layerRemoved.count(), 1);
        QCOMPARE(layers->getLayersForNode(node->id).size(), 1);
        viewController->removeView(first);
        QCOMPARE(removed.count(), 1);
        registry->removeObject(node->id);
        QCOMPARE(layerRemoved.count(), 2);
        QVERIFY(layers->getAllLayers().isEmpty());
        viewController->removeView(second);
        QCOMPARE(removed.count(), 2);
    }

    void testSnapshotSelectionAndPrefetch() {
        auto experiment = std::make_shared<Experiment>("Experiment");
        auto early = std::make_shared<Snapshot>("Early", 1.0);
        auto late = std::make_shared<Snapshot>("Late", 5.0);
        auto first = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "First", 1.0);
        auto last = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Last", 5.0);
        auto unloaded = std::make_shared<DataNode>(nullptr, "Unloaded", 5.0);
        first->type = QSpace::Visualize::EntityType::Gas;
        last->type = QSpace::Visualize::EntityType::Gas;
        unloaded->type = QSpace::Visualize::EntityType::Stars;
        early->addComponent(first);
        late->addComponent(last);
        late->addComponent(unloaded);
        experiment->addSnapshot(late);
        experiment->addSnapshot(early);
        registry->registerExperiment(experiment);
        const QUuid viewId = viewController->createView(ViewType::OpenGL3D);
        QVERIFY(!viewId.isNull());
        const auto sourceLayers = layers->getLayersForNode(first->id);
        QCOMPARE(sourceLayers.size(), 1);
        sourceLayers.front()->getSettings()->setColorByField("TestField");
        QSignalSpy sizeSpy(video.get(), &VideoController::snapshotsListSizeChanged);
        QSignalSpy loadSpy(video.get(), &VideoController::requestNodeLoad);
        video->handleTargetExperimentChange(experiment->id);
        QCOMPARE(sizeSpy.count(), 1);
        QCOMPARE(sizeSpy.first().first().toInt(), 2);
        QCOMPARE(video->getCurrentSnapshotId(), early->id);
        QVERIFY(loadSpy.count() >= 1);
        QCOMPARE(loadSpy.first().first().value<QUuid>(), unloaded->id);
        auto allLayers = layers->getAllLayers();
        std::shared_ptr<QSpace::Visualize::Layers::Layer> playback;
        for (const auto& layer : allLayers)
            if (layer->layerId() != sourceLayers.front()->layerId() && layer->dataNodeId() == first->id) playback = layer;
        QVERIFY(playback);
        QCOMPARE(playback->getSettings()->colorByField(), QString("TestField"));
        video->handleActivateVisualizeSnapshot(2);
        QCOMPARE(video->getCurrentSnapshotId(), late->id);
        QCOMPARE(playback->dataNodeId(), last->id);
        QCOMPARE(playback->getSettings()->colorByField(), QString("TestField"));
        video->handleActivateVisualizeSnapshot(0);
        video->handleActivateVisualizeSnapshot(3);
        QCOMPARE(video->getCurrentSnapshotId(), late->id);
        video->handleActivateVisualizeSnapshot(1);
        QCOMPARE(video->getCurrentSnapshotId(), early->id);
        QCOMPARE(playback->dataNodeId(), first->id);
    }
  private:
    std::unique_ptr<ObjectRegistry> registry;
    std::unique_ptr<LayerManager> layers;
    std::unique_ptr<ViewManager> views;
    std::unique_ptr<ViewController> viewController;
    std::unique_ptr<VideoController> video;
};

QTEST_MAIN(TestVideoViewController)
#include "TestVideoViewController.moc"
