#include <QSignalSpy>
#include <QtTest>
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

class TestRemovalNode : public QObject {
    Q_OBJECT

  private slots:

    // Вызывается перед каждым тестом: создаем чистый реестр
    void init() {
        m_registry = std::make_unique<ObjectRegistry>();
    }

    // Тест 3: Удаление
    void testRemovalNode_Success() {
        auto  polyData = vtkSmartPointer<vtkPolyData>::New();
        auto  node     = std::make_shared<DataNode>(polyData, "DeleteMe");
        QUuid id       = node->id;
        m_registry->registerNode(node);

        QSignalSpy spy(m_registry.get(), &ObjectRegistry::objectRemoved);

        m_registry->removeObject(id);

        QCOMPARE(m_registry->getAllNodes().size(), 0);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).value<QUuid>(), id);
    }

    void testRemovalNode_NonExistent() {
        QUuid nonExistentId = QUuid::createUuid();

        QSignalSpy spy(m_registry.get(), &ObjectRegistry::objectRemoved);

        m_registry->removeObject(nonExistentId);

        QCOMPARE(m_registry->getAllNodes().size(), 0);
        QCOMPARE(spy.count(), 0); // Сигнал не должен быть вызван
    }

    void testRemoveExperimentCascadesToSnapshotsAndNodes() {
        auto experiment = std::make_shared<Experiment>("Experiment");
        auto firstSnapshot = std::make_shared<Snapshot>("First", 1.0);
        auto secondSnapshot = std::make_shared<Snapshot>("Second", 2.0);
        auto firstNode = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "First node");
        auto secondNode = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Second node");
        firstSnapshot->addComponent(firstNode);
        secondSnapshot->addComponent(secondNode);
        experiment->addSnapshot(firstSnapshot);
        experiment->addSnapshot(secondSnapshot);
        m_registry->registerExperiment(experiment);

        QSignalSpy removed(m_registry.get(), &ObjectRegistry::objectRemoved);
        m_registry->removeObject(experiment->id);

        QCOMPARE(m_registry->experimentCount(), 0);
        QCOMPARE(m_registry->snapshotCount(), 0);
        QCOMPARE(m_registry->nodeCount(), 0);
        QCOMPARE(removed.count(), 5);
        QVERIFY(!m_registry->getExperiment(experiment->id));
        QVERIFY(!m_registry->getSnapshot(firstSnapshot->id));
        QVERIFY(!m_registry->getSnapshot(secondSnapshot->id));
        QVERIFY(!m_registry->getNode(firstNode->id));
        QVERIFY(!m_registry->getNode(secondNode->id));
    }

    void testRemoveSnapshotCascadesToNodesAndUnlinksExperiment() {
        auto experiment = std::make_shared<Experiment>("Experiment");
        auto removableSnapshot = std::make_shared<Snapshot>("Remove", 1.0);
        auto retainedSnapshot = std::make_shared<Snapshot>("Keep", 2.0);
        auto removableNode = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Remove node");
        auto retainedNode = std::make_shared<DataNode>(vtkSmartPointer<vtkPolyData>::New(), "Keep node");
        removableSnapshot->addComponent(removableNode);
        retainedSnapshot->addComponent(retainedNode);
        experiment->addSnapshot(removableSnapshot);
        experiment->addSnapshot(retainedSnapshot);
        m_registry->registerExperiment(experiment);

        QSignalSpy removed(m_registry.get(), &ObjectRegistry::objectRemoved);
        m_registry->removeObject(removableSnapshot->id);

        QCOMPARE(m_registry->experimentCount(), 1);
        QCOMPARE(m_registry->snapshotCount(), 1);
        QCOMPARE(m_registry->nodeCount(), 1);
        QCOMPARE(removed.count(), 2);
        QCOMPARE(experiment->snapshots.size(), 1);
        QCOMPARE(experiment->snapshots.front()->id, retainedSnapshot->id);
        QVERIFY(!m_registry->getSnapshot(removableSnapshot->id));
        QVERIFY(!m_registry->getNode(removableNode->id));
        QVERIFY(m_registry->getNode(retainedNode->id));
    }

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
};

// Не забудь зарегистрировать shared_ptr в метасистеме,
// если это еще не сделано в основном коде, иначе QSignalSpy его не вытащит.
Q_DECLARE_METATYPE(std::shared_ptr<QSpace::Core::DataNode>)

QTEST_MAIN(TestRemovalNode)
#include "TestRemovalNode.moc"