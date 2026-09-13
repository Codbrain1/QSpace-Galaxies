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

class TestRegisterContainer : public QObject {
    Q_OBJECT

  private slots:

    // Вызывается перед каждым тестом: создаем чистый реестр
    void init() {
        m_registry = std::make_unique<ObjectRegistry>();
    }

    void testContainerRegister_Succes() {
        QSignalSpy spy(m_registry.get(), &ObjectRegistry::snapshotAdded);
        auto       container = std::make_shared<QSpace::Core::Snapshot>("TestContainer");
        m_registry->registerSnapshot(container);
        QCOMPARE(m_registry->getAllSnapshots().size(), 1);
        QCOMPARE(spy.count(), 1);
        auto signalContainer = spy.at(0).at(0).value<std::shared_ptr<QSpace::Core::Snapshot>>();
        QCOMPARE(signalContainer->id, container->id);
    }

    void testContainerRegister_NullPointer() {
        m_registry->registerSnapshot(nullptr);
        QCOMPARE(m_registry->getAllSnapshots().size(), 0);
    }

  private:
    std::unique_ptr<ObjectRegistry> m_registry;
};

// Не забудь зарегистрировать shared_ptr в метасистеме,
// если это еще не сделано в основном коде, иначе QSignalSpy его не вытащит.
Q_DECLARE_METATYPE(std::shared_ptr<QSpace::Core::DataNode>)

QTEST_MAIN(TestRegisterContainer)
#include "TestRegisterContainer.moc"