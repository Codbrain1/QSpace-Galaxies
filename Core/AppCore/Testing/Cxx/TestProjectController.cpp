#include <QDataStream>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "Core/AppCore/DataController.h"
#include "Core/AppCore/ProjectController.h"
#include "Core/DataManager/DataManager.h"
#include "Core/FileSchemeRegistry/FileSchemeRegistry.h"
#include "Core/LayerManager/LayerManager.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Core/SessionManager/SessionManager.h"
#include "Core/TaskManager/TaskManager.h"
#include "Core/ViewManager/ViewManager.h"

using namespace QSpace::Core;
using namespace QSpace::Core::Controllers;

class TestProjectController : public QObject {
    Q_OBJECT
  private slots:
    void init() {
        tasks = std::make_unique<TaskManager>();
        data = std::make_unique<DataManager>(tasks.get());
        registry = std::make_unique<ObjectRegistry>();
        layers = std::make_unique<LayerManager>();
        views = std::make_unique<ViewManager>();
        schemes = std::make_unique<FileSchemeRegistry>();
        schemes->registerStandartPresets();
        controller = std::make_unique<DataController>(data.get(), registry.get(), layers.get(), views.get(), schemes.get());
        controller->initialize();
        session = std::make_unique<SessionManager>(registry.get(), layers.get(), views.get());
        project = std::make_unique<ProjectController>(session.get(), registry.get(), data.get(), controller.get());
        project->initialize();
    }

    void testDirtyStateAndSaveFailures() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSignalSpy states(project.get(), &ProjectController::sessionStateChanged);
        QSignalSpy request(project.get(), &ProjectController::requestSavePathFromUI);
        QSignalSpy errors(session.get(), &SessionManager::errorOccurred);
        project->createNewProject("Test");
        QCOMPARE(states.count(), 1);
        QVERIFY(!states.last().at(0).value<QSpace::Session::CurrentSession>().isDirty);
        auto node = std::make_shared<DataNode>(nullptr, "Node");
        registry->registerNode(node);
        controller->removeNodeObject(node->id);
        QVERIFY(states.last().at(0).value<QSpace::Session::CurrentSession>().isDirty);
        project->saveCurrentProject();
        QCOMPARE(request.count(), 1);
        project->saveCurrentProjectAs(dir.filePath("missing/project.json"));
        QCOMPARE(errors.count(), 1);
        QVERIFY(states.last().at(0).value<QSpace::Session::CurrentSession>().isDirty);
        const QString path = dir.filePath("project.json");
        project->saveCurrentProjectAs(path);
        QVERIFY(QFile::exists(path));
        QVERIFY(!states.last().at(0).value<QSpace::Session::CurrentSession>().isDirty);
        QCOMPARE(states.last().at(0).value<QSpace::Session::CurrentSession>().projectFilePath, path);
    }

    void testOpenFailurePreservesExistingProject() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        project->createNewProject("Existing");
        auto node = std::make_shared<DataNode>(nullptr, "Existing node");
        registry->registerNode(node);
        QSignalSpy states(project.get(), &ProjectController::sessionStateChanged);
        QSignalSpy errors(session.get(), &SessionManager::errorOccurred);
        project->openProject(dir.filePath("missing.json"));
        QCOMPARE(errors.count(), 1);
        QCOMPARE(states.count(), 0);
        QVERIFY(registry->getNode(node->id));
    }

    void testOpenProjectRestoresAndLoadsData() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString binPath = dir.filePath("Gas_restore.bin");
        QFile file(binPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << qint32(1) << double(2.5);
        for (int i = 0; i < 9; ++i) stream << double(i);
        stream << qint32(0);
        file.close();
        auto node = std::make_shared<DataNode>(nullptr, "Restored");
        node->path = binPath;
        node->format = QSpace::IO::FileFormat::BIN;
        node->type = QSpace::Visualize::EntityType::Gas;
        registry->registerNode(node);
        const QUuid id = node->id;
        const QString path = dir.filePath("project.json");
        project->saveCurrentProjectAs(path);
        QVERIFY(QFile::exists(path));
        project->createNewProject("Other");
        QCOMPARE(registry->nodeCount(), 0);
        QSignalSpy loaded(controller.get(), &DataController::nodeDataLoaded);
        QSignalSpy states(project.get(), &ProjectController::sessionStateChanged);
        project->openProject(path);
        QCOMPARE(states.count(), 1);
        QVERIFY(!states.last().at(0).value<QSpace::Session::CurrentSession>().isDirty);
        QCOMPARE(states.last().at(0).value<QSpace::Session::CurrentSession>().projectFilePath, path);
        QTRY_VERIFY(registry->getNode(id));
        QTRY_VERIFY(registry->getNode(id)->data != nullptr);
        QCOMPARE(loaded.count(), 1);
    }
  private:
    std::unique_ptr<TaskManager> tasks;
    std::unique_ptr<DataManager> data;
    std::unique_ptr<ObjectRegistry> registry;
    std::unique_ptr<LayerManager> layers;
    std::unique_ptr<ViewManager> views;
    std::unique_ptr<FileSchemeRegistry> schemes;
    std::unique_ptr<DataController> controller;
    std::unique_ptr<SessionManager> session;
    std::unique_ptr<ProjectController> project;
};

QTEST_MAIN(TestProjectController)
#include "TestProjectController.moc"
