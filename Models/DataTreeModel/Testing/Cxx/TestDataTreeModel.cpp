#include <QtTest>
#include <vtkPolyData.h>

#include "Common/Structures/ObjectRegistryStructures.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/ViewManager/ViewManager.h"
#include "Common/Enums/ViewEnums.h"
#include "Models/DataTreeModel/DataTreeModel.h"

using namespace QSpace::Core;
using namespace QSpace::Models;

class TestDataTreeModel : public QObject {
    Q_OBJECT

  private slots:
    void init() {
        m_registry     = std::make_unique<ObjectRegistry>();
        m_layerManager = std::make_unique<LayerManager>();
        m_model = std::make_unique<DataTreeModel>(m_registry.get(), m_layerManager.get());
    }

    void testRegistryAdditionsBuildNavigableSnapshotTree() {
        auto experiment = std::make_shared<Experiment>(QStringLiteral("Experiment"));
        m_registry->registerExperiment(experiment);

        QCOMPARE(m_model->rowCount(), 1);
        const QModelIndex experimentIndex = m_model->index(0, DataTreeModel::NameColumn);
        QVERIFY(experimentIndex.isValid());
        QCOMPARE(experimentIndex.data(DataTreeModel::IdRole).value<QUuid>(), experiment->id);
        QVERIFY(!m_model->parent(experimentIndex).isValid());

        auto snapshot = std::make_shared<Snapshot>(QStringLiteral("Snapshot"));
        experiment->addSnapshot(snapshot);
        m_registry->registerSnapshot(snapshot, experiment->id);

        QCOMPARE(m_model->rowCount(experimentIndex), 1);
        const QModelIndex snapshotIndex = m_model->index(0, DataTreeModel::NameColumn, experimentIndex);
        QVERIFY(snapshotIndex.isValid());
        QCOMPARE(m_model->parent(snapshotIndex), experimentIndex);
        QCOMPARE(snapshotIndex.data(DataTreeModel::IdRole).value<QUuid>(), snapshot->id);

        auto node = std::make_shared<DataNode>(nullptr, QStringLiteral("Gas node"));
        node->type = QSpace::Visualize::EntityType::Gas;
        snapshot->addComponent(node);
        m_registry->registerNodeToSnapshot(node, snapshot->id);

        QCOMPARE(m_model->rowCount(snapshotIndex), 1);
        const QModelIndex nodeIndex = m_model->index(0, DataTreeModel::NameColumn, snapshotIndex);
        QVERIFY(nodeIndex.isValid());
        QCOMPARE(m_model->parent(nodeIndex), snapshotIndex);
        QCOMPARE(nodeIndex.data(DataTreeModel::IdRole).value<QUuid>(), node->id);
        QCOMPARE(m_model->rowCount(m_model->index(0, DataTreeModel::MemoryColumn)), 0);
        QVERIFY(!m_model->index(1, DataTreeModel::NameColumn, snapshotIndex).isValid());
    }

    void testRegistryRemovalsUpdateTreeBranches() {
        auto experiment = std::make_shared<Experiment>(QStringLiteral("Experiment"));
        auto snapshot   = std::make_shared<Snapshot>(QStringLiteral("Snapshot"));
        auto node       = std::make_shared<DataNode>(nullptr, QStringLiteral("Node"));
        snapshot->addComponent(node);
        experiment->addSnapshot(snapshot);

        m_registry->registerExperiment(experiment);
        m_registry->registerSnapshot(snapshot, experiment->id);
        m_registry->registerNodeToSnapshot(node, snapshot->id);

        const QModelIndex experimentIndex = m_model->index(0, DataTreeModel::NameColumn);
        const QModelIndex snapshotIndex = m_model->index(0, DataTreeModel::NameColumn, experimentIndex);
        QCOMPARE(m_model->rowCount(snapshotIndex), 1);

        m_registry->removeObject(node->id);
        QCOMPARE(m_model->rowCount(snapshotIndex), 0);

        m_registry->removeObject(snapshot->id);
        QCOMPARE(m_model->rowCount(experimentIndex), 0);

        m_registry->removeObject(experiment->id);
        QCOMPARE(m_model->rowCount(), 0);
    }

    void testRefreshesNodeWhenRegistryUpdatesItsData() {
        auto experiment = std::make_shared<Experiment>(QStringLiteral("Experiment"));
        auto snapshot = std::make_shared<Snapshot>(QStringLiteral("Snapshot"));
        auto node = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), QStringLiteral("Node"));
        snapshot->addComponent(node);
        experiment->addSnapshot(snapshot);
        m_registry->registerExperiment(experiment);
        m_registry->registerSnapshot(snapshot, experiment->id);
        m_registry->registerNodeToSnapshot(node, snapshot->id);

        const QModelIndex experimentIndex = m_model->index(0, DataTreeModel::NameColumn);
        const QModelIndex snapshotIndex = m_model->index(0, DataTreeModel::NameColumn, experimentIndex);
        const QModelIndex nodeIndex = m_model->index(0, DataTreeModel::NameColumn, snapshotIndex);
        QSignalSpy changedSpy(m_model.get(), &QAbstractItemModel::dataChanged);

        m_registry->updateNodeData(node->id, nullptr, node->stats.timestamp);

        QCOMPARE(changedSpy.count(), 1);
        const auto changed = changedSpy.front();
        QCOMPARE(changed.at(0).value<QModelIndex>(), nodeIndex);
        QCOMPARE(changed.at(1).value<QModelIndex>(),
                 m_model->index(nodeIndex.row(), m_model->columnCount(snapshotIndex) - 1, snapshotIndex));
    }

    void testEditingAndVisibilityPropagation() {
        auto experiment = std::make_shared<Experiment>("Experiment");
        auto snapshot = std::make_shared<Snapshot>("Snapshot");
        auto first = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "First");
        auto second = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Second");
        snapshot->addComponent(first);
        snapshot->addComponent(second);
        experiment->addSnapshot(snapshot);
        m_registry->registerExperiment(experiment);
        ViewManager views;
        const auto viewId = views.createView(QSpace::Visualize::Views::ViewType::OpenGL3D);
        QVERIFY(!viewId.isNull());
        const auto firstLayerId = m_layerManager->createLayer(first, views.getView(viewId));
        const auto secondLayerId = m_layerManager->createLayer(second, views.getView(viewId));
        QVERIFY(!firstLayerId.isNull());
        QVERIFY(!secondLayerId.isNull());
        const auto expIndex = m_model->index(0, 0);
        const auto snapshotIndex = m_model->index(0, 0, expIndex);
        const auto firstIndex = m_model->index(0, 0, snapshotIndex);
        const auto secondIndex = m_model->index(1, 0, snapshotIndex);
        const auto layerIndex = m_model->index(0, 0, firstIndex);
        QVERIFY(layerIndex.isValid());
        QSignalSpy changed(m_model.get(), &QAbstractItemModel::dataChanged);
        QSignalSpy scene(m_model.get(), &DataTreeModel::sceneUpdateRequested);
        QVERIFY(m_model->setData(expIndex, " Renamed ", Qt::EditRole));
        QCOMPARE(experiment->name, QString("Renamed"));
        QVERIFY(!m_model->setData(expIndex, "  ", Qt::EditRole));
        QVERIFY(m_model->setData(layerIndex, "Layer renamed", Qt::EditRole));
        QCOMPARE(m_layerManager->getLayer(firstLayerId)->name(), QString("Layer renamed"));
        QVERIFY(m_model->setData(firstIndex, Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(!m_layerManager->getLayer(firstLayerId)->getSettings()->isVisible());
        QVERIFY(m_layerManager->getLayer(secondLayerId)->getSettings()->isVisible());
        QCOMPARE(snapshotIndex.data(Qt::CheckStateRole).toInt(), int(Qt::PartiallyChecked));
        QCOMPARE(expIndex.data(Qt::CheckStateRole).toInt(), int(Qt::PartiallyChecked));
        QVERIFY(m_model->setData(snapshotIndex, Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(!m_layerManager->getLayer(secondLayerId)->getSettings()->isVisible());
        QVERIFY(m_model->setData(expIndex, Qt::Checked, Qt::CheckStateRole));
        QVERIFY(m_layerManager->getLayer(firstLayerId)->getSettings()->isVisible());
        QVERIFY(m_layerManager->getLayer(secondLayerId)->getSettings()->isVisible());
        QVERIFY(m_model->setData(layerIndex, Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(!m_layerManager->getLayer(firstLayerId)->getSettings()->isVisible());
        QVERIFY(secondIndex.isValid());
        QVERIFY(changed.count() >= 4);
        QCOMPARE(scene.count(), 4);
    }

    void testComponentViewRebuildsAndTracksNewNodes() {
        auto experiment = std::make_shared<Experiment>(QStringLiteral("Experiment"));
        auto snapshot = std::make_shared<Snapshot>(QStringLiteral("Snapshot"));
        experiment->addSnapshot(snapshot);
        m_registry->registerExperiment(experiment);
        m_registry->registerSnapshot(snapshot, experiment->id);

        m_model->setTreeMode(DataTreeModel::TreeMode::ComponentView);
        const QModelIndex experimentIndex = m_model->index(0, DataTreeModel::NameColumn);
        QCOMPARE(m_model->rowCount(experimentIndex), 0);

        auto node = std::make_shared<DataNode>(nullptr, QStringLiteral("Stars node"));
        node->type = QSpace::Visualize::EntityType::Stars;
        snapshot->addComponent(node);
        m_registry->registerNodeToSnapshot(node, snapshot->id);

        QCOMPARE(m_model->rowCount(experimentIndex), 1);
        const QModelIndex groupIndex = m_model->index(0, DataTreeModel::NameColumn, experimentIndex);
        QCOMPARE(m_model->rowCount(groupIndex), 1);
        const QModelIndex nodeIndex = m_model->index(0, DataTreeModel::NameColumn, groupIndex);
        QCOMPARE(nodeIndex.data(DataTreeModel::IdRole).value<QUuid>(), node->id);
    }

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
    std::unique_ptr<LayerManager>   m_layerManager;
    std::unique_ptr<DataTreeModel>  m_model;
};

QTEST_MAIN(TestDataTreeModel)
#include "TestDataTreeModel.moc"
