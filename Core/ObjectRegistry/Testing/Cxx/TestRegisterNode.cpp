#include <QSignalSpy>
#include <QtTest>
#include <cstddef>
#include <memory>

// VTK включает для создания тестовых данных
#include <qsignalspy.h>
#include <qtestcase.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

// Твои заголовки
#include "Common/Structures/ObjectRegistryStructures.h" // Путь к файлу с DataNode
#include "Core/ObjectRegistry/ObjectRegistry.h"

using namespace QSpace::Core;

class TestRegisterNode : public QObject {
    Q_OBJECT

  private slots:

    // Вызывается перед каждым тестом: создаем чистый реестр
    void init() {
        m_registry = std::make_unique<ObjectRegistry>();
    }

    void testRegisterNode_Success() {
        QSignalSpy spy(m_registry.get(), &ObjectRegistry::nodeAdded);
        auto       dummyData = vtkSmartPointer<vtkPolyData>::New();
        auto       node      = std::make_shared<DataNode>(dummyData, "TestNode");
        m_registry->registerNode(node);
        QCOMPARE(m_registry->getAllNodes().size(), 1);
        QCOMPARE(spy.count(), 1);
        auto signaledNode = spy.at(0).at(0).value<std::shared_ptr<QSpace::Core::DataNode>>();
        QCOMPARE(signaledNode->id, node->id);
    }

    void testRegisterNode_NullPointer() {
        QSignalSpy spy(m_registry.get(), &ObjectRegistry::nodeAdded);
        m_registry->registerNode(nullptr);
        QCOMPARE(m_registry->getAllNodes().size(), 0);
        QCOMPARE(spy.count(), 0);
    }

    void testRegisterNode_NullPointerData() {
        QSignalSpy spy(m_registry.get(), &ObjectRegistry::nodeAdded);
        auto       node = std::make_shared<DataNode>(nullptr, "TestNode");
        m_registry->registerNode(node);
        QCOMPARE(m_registry->getAllNodes().size(), 1);
        QCOMPARE(spy.count(), 1);
    }

    void testRegisterNodeToExperimentGroupsMatchingTimestamps() {
        auto experiment = std::make_shared<Experiment>("Experiment");
        m_registry->registerExperiment(experiment);

        QSignalSpy snapshots(m_registry.get(), &ObjectRegistry::snapshotAdded);
        QSignalSpy nodes(m_registry.get(), &ObjectRegistry::nodeAdded);
        auto first = std::make_shared<DataNode>(nullptr, "First", 10.0);
        auto matching = std::make_shared<DataNode>(nullptr, "Matching", 10.0 + 0.000005);
        auto distinct = std::make_shared<DataNode>(nullptr, "Distinct", 10.00002);

        m_registry->registerNodeToExperiment(first, experiment->id);
        m_registry->registerNodeToExperiment(matching, experiment->id);
        m_registry->registerNodeToExperiment(distinct, experiment->id);

        QCOMPARE(experiment->snapshots.size(), 2);
        QCOMPARE(m_registry->snapshotCount(), 2);
        QCOMPARE(m_registry->nodeCount(), 3);
        QCOMPARE(snapshots.count(), 2);
        QCOMPARE(nodes.count(), 3);
        QCOMPARE(experiment->snapshots.front()->components.size(), 2);
        QCOMPARE(experiment->snapshots.front()->timestamp, 10.0);
        QCOMPARE(experiment->snapshots.back()->components.size(), 1);
        QCOMPARE(experiment->snapshots.back()->timestamp, 10.00002);
    }

    void testLruCacheEvictsLeastRecentlyUsedNodeAndUpdatesData() {
        m_registry->setCacheCapacity(2);
        auto first = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "First");
        auto second = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Second");
        auto third = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Third");
        m_registry->registerNode(first);
        m_registry->registerNode(second);

        QSignalSpy updates(m_registry.get(), &ObjectRegistry::nodeDataUpdated);
        QVERIFY(m_registry->getOrLoadNodeData(first->id) == first);
        m_registry->registerNode(third);

        QVERIFY(first->data);
        QVERIFY(!second->data);
        QVERIFY(third->data);
        QCOMPARE(updates.count(), 1);
        QCOMPARE(updates.front().front().value<QUuid>(), second->id);

        auto replacement = vtkSmartPointer<vtkPolyData>::New();
        m_registry->updateNodeData(first->id, replacement, 42.0);

        QVERIFY(first->data == replacement);
        QCOMPARE(first->stats.timestamp, 42.0);
    }

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
};

// Не забудь зарегистрировать shared_ptr в метасистеме,
// если это еще не сделано в основном коде, иначе QSignalSpy его не вытащит.
Q_DECLARE_METATYPE(std::shared_ptr<QSpace::Core::DataNode>)

QTEST_MAIN(TestRegisterNode)
#include "TestRegisterNode.moc"