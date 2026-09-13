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

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
};

// Не забудь зарегистрировать shared_ptr в метасистеме,
// если это еще не сделано в основном коде, иначе QSignalSpy его не вытащит.
Q_DECLARE_METATYPE(std::shared_ptr<QSpace::Core::DataNode>)

QTEST_MAIN(TestRemovalNode)
#include "TestRemovalNode.moc"