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

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
};

// Не забудь зарегистрировать shared_ptr в метасистеме,
// если это еще не сделано в основном коде, иначе QSignalSpy его не вытащит.
Q_DECLARE_METATYPE(std::shared_ptr<QSpace::Core::DataNode>)

QTEST_MAIN(TestRegisterNode)
#include "TestRegisterNode.moc"