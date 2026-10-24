#include <QVariantMap>
#include <QtTest>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include "Common/Structures/ObjectRegistryStructures.h"
#include "Common/Structures/SessionStructures.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/LayerManager/LayerManagerSerializer.h"
#include "Core/ViewManager/ViewManager.h"
#include "Core/ViewManager/ViewManagerSerializer.h"
#include "Session/Reflection.h"

using namespace QSpace::Core;
using namespace QSpace::Session;
using namespace QSpace::Visualize::Views;
using QSpace::Visualize::Layers::RenderLayerType;

class TestLayerManagerSerializer : public QObject {
    Q_OBJECT

  private slots:
    void testNullManagerAndMissingResolvers() {
        QVERIFY(!LayerManagerSerializer::toDTO(nullptr).has_value());
        LayerManagerDTO dto;
        LayerDTO layer;
        layer.layerId = QUuid::createUuid();
        layer.nodeId = QUuid::createUuid();
        layer.viewId = QUuid::createUuid();
        layer.renderType = RenderLayerType::SPH;
        dto.layers.append(layer);
        LayerManager manager;
        QVERIFY(!LayerManagerSerializer::fromDTO(dto, nullptr, {}, {}));
        QVERIFY(!LayerManagerSerializer::fromDTO(dto, &manager,
                                                  [](const QUuid&) { return std::shared_ptr<DataNode>(); },
                                                  [](const QUuid&) { return std::shared_ptr<AbstractView>(); }));
        QVERIFY(manager.getAllLayers().isEmpty());
    }

    void testUnknownNodeAndViewDoNotCreateRenderer() {
        LayerDTO layer;
        layer.layerId = QUuid::createUuid();
        layer.nodeId = QUuid::createUuid();
        layer.viewId = QUuid::createUuid();
        layer.renderType = RenderLayerType::SPH;
        QVERIFY(!LayerManagerSerializer::fromDTO(layer,
                                                  [](const QUuid&) { return std::shared_ptr<DataNode>(); },
                                                  [](const QUuid&) { return std::shared_ptr<AbstractView>(); }));
    }

    void testUnknownRenderTypeIsRejected() {
        const auto node = makeNode();
        ViewManager views;
        const auto viewId = views.createView(ViewType::OpenGL3D);
        if (viewId.isNull()) QSKIP("OpenGL view is unavailable");
        LayerDTO layer;
        layer.layerId = QUuid::createUuid();
        layer.nodeId = node->id;
        layer.viewId = viewId;
        layer.renderType = static_cast<RenderLayerType>(999);
        QVERIFY(!LayerManagerSerializer::fromDTO(layer,
                                                  [node](const QUuid& id) { return id == node->id ? node : nullptr; },
                                                  [&views](const QUuid& id) { return views.getView(id); }));
    }

    void testRoundTripRestoresMultipleLayersAndState() {
        ViewManager sourceViews;
        LayerManager sourceLayers;
        const auto viewId = sourceViews.createView(ViewType::OpenGL3D);
        if (viewId.isNull()) QSKIP("OpenGL view is unavailable");
        const auto view = sourceViews.getView(viewId);
        const auto firstNode = makeNode();
        const auto secondNode = makeNode();
        const auto firstId = sourceLayers.createLayer(firstNode, view);
        const auto secondId = sourceLayers.createLayer(secondNode, view, RenderLayerType::Particles);
        if (firstId.isNull() || secondId.isNull()) QSKIP("Layer renderer is unavailable");
        auto first = sourceLayers.getLayer(firstId);
        auto second = sourceLayers.getLayer(secondId);
        first->setName("Renamed layer");
        first->setIsSyncedWithMaster(false);
        second->setName("Particle layer");
        if (auto settings = first->getSettings()) settings->setColorByField("Mass");

        const auto dto = LayerManagerSerializer::toDTO(&sourceLayers);
        QVERIFY(dto.has_value());
        QCOMPARE(dto->layers.size(), 2);

        LayerManager restored;
        QVERIFY(LayerManagerSerializer::fromDTO(
            *dto, &restored,
            [firstNode, secondNode](const QUuid& id) {
                if (id == firstNode->id) return firstNode;
                if (id == secondNode->id) return secondNode;
                return std::shared_ptr<DataNode>();
            },
            [&sourceViews](const QUuid& id) { return sourceViews.getView(id); }));
        QCOMPARE(restored.getAllLayers().size(), 2);
        const auto restoredFirst = restored.getLayer(firstId);
        QVERIFY(restoredFirst);
        QCOMPARE(restoredFirst->name(), QString("Renamed layer"));
        QCOMPARE(restoredFirst->IsSyncedWithMaster(), false);
        QCOMPARE(restoredFirst->getEngine()->type(), RenderLayerType::SPH);
        QCOMPARE(restored.getLayer(secondId)->getEngine()->type(), RenderLayerType::Particles);
    }

  private:
    static std::shared_ptr<DataNode> makeNode() {
        auto data = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->InsertNextPoint(0.0, 0.0, 0.0);
        data->SetPoints(points);
        return std::make_shared<DataNode>(data, "Test node");
    }
};

QTEST_MAIN(TestLayerManagerSerializer)
#include "TestLayerManagerSerializer.moc"
