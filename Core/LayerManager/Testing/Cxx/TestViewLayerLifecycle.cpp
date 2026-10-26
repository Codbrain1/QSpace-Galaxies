#include <QSignalSpy>
#include <QtTest>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include "Common/Structures/ObjectRegistryStructures.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ViewManager/ViewManager.h"

using namespace QSpace::Core;
using namespace QSpace::Visualize::Views;
using QSpace::Visualize::Layers::RenderLayerType;

class TestViewLayerLifecycle : public QObject {
    Q_OBJECT

  private slots:
    void testRejectsInvalidInputs() {
        LayerManager manager;
        ViewManager views;
        const auto viewId = views.createView(ViewType::OpenGL3D);
        if (viewId.isNull()) QSKIP("OpenGL view is unavailable");
        const auto view = views.getView(viewId);
        const auto loaded = makeNode(true);
        const auto unloaded = makeNode(false);

        QVERIFY(manager.createLayer(nullptr, view).isNull());
        QVERIFY(manager.createLayer(loaded, nullptr).isNull());
        QVERIFY(manager.createLayer(unloaded, view).isNull());
        QVERIFY(manager.createLayer(loaded, view, static_cast<RenderLayerType>(999)).isNull());
        QCOMPARE(manager.getAllLayers().size(), 0);
    }

    void testMultipleLayersMaintainNodeAndViewIndexes() {
        LayerManager manager;
        ViewManager views;
        const auto viewId = views.createView(ViewType::OpenGL3D);
        if (viewId.isNull()) QSKIP("OpenGL view is unavailable");
        const auto view = views.getView(viewId);
        const auto node = makeNode(true);

        const auto first = manager.createLayer(node, view);
        const auto second = manager.createLayer(node, view);
        if (first.isNull() || second.isNull()) QSKIP("Layer renderer is unavailable");
        QVERIFY(first != second);
        QCOMPARE(manager.getAllLayers().size(), 2);
        QCOMPARE(manager.getLayersForNode(node->id).size(), 2);

        manager.removeLayer(QUuid::createUuid());
        QCOMPARE(manager.getAllLayers().size(), 2);
        manager.removeLayer(first);
        QCOMPARE(manager.getAllLayers().size(), 1);
        QCOMPARE(manager.getLayersForNode(node->id).size(), 1);
        QCOMPARE(manager.getLayersForNode(node->id).first()->layerId(), second);
        manager.removeLayer(first);
        manager.removeAllLayersForNode(node->id);
        QVERIFY(manager.getAllLayers().isEmpty());
    }

    void testRemovesAllLayersForViewAndDetachesRenderers() {
        LayerManager manager;
        ViewManager views;
        const auto firstViewId = views.createView(ViewType::OpenGL3D);
        const auto secondViewId = views.createView(ViewType::OpenGL3D);
        if (firstViewId.isNull() || secondViewId.isNull()) QSKIP("OpenGL views are unavailable");
        const auto firstView = views.getView(firstViewId);
        const auto secondView = views.getView(secondViewId);
        const auto node = makeNode(true);

        const auto first = manager.createLayer(node, firstView);
        const auto second = manager.createLayer(node, secondView);
        if (first.isNull() || second.isNull()) QSKIP("Layer renderer is unavailable");
        manager.removeAllLayersForView(firstView.get());
        QVERIFY(!manager.getLayer(first));
        QVERIFY(manager.getLayer(second));
        QCOMPARE(manager.getLayersForNode(node->id).size(), 1);

        manager.removeLayer(QUuid::createUuid());
        manager.clear();
        QVERIFY(manager.getAllLayers().isEmpty());
        QVERIFY(manager.getLayersForNode(node->id).isEmpty());
    }

  private:
    static std::shared_ptr<DataNode> makeNode(bool loaded) {
        vtkSmartPointer<vtkPolyData> data;
        if (loaded) {
            data = vtkSmartPointer<vtkPolyData>::New();
            auto points = vtkSmartPointer<vtkPoints>::New();
            points->InsertNextPoint(0.0, 0.0, 0.0);
            data->SetPoints(points);
        }
        return std::make_shared<DataNode>(data, loaded ? "Loaded node" : "Unloaded node");
    }
};

QTEST_MAIN(TestViewLayerLifecycle)
#include "TestViewLayerLifecycle.moc"
