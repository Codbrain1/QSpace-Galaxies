#include "Common/Enums/ViewEnums.h"
#include "Common/Structures/SessionStructures.h"
#include "Core/ViewManager/ViewManager.h"
#include "Core/ViewManager/ViewManagerSerializer.h"
#include <QSignalSpy>
#include <QtTest>

using namespace QSpace::Core;
using namespace QSpace::Session;
using namespace QSpace::Visualize::Views;

class TestViewManagerSerialize : public QObject {
    Q_OBJECT

    static ViewDTO makeOpenGLViewDTO(const QUuid& id, const QString& name = "View") {
        ViewDTO view;
        view.id = id;
        view.name = name;
        view.type = ViewType::OpenGL3D;
        return view;
    }

    static QUuid createOpenGLView(ViewManager& manager) {
        return manager.createView(ViewType::OpenGL3D);
    }

  private slots:
    void testSerializeEmptyManager();
    void testCreateMultipleViewsAndSerializeNameAndSettings();
    void testRemoveMainView();
    void testRemoveRegularViewAndUnknownId();
    void testSetUnknownViewAsMainDoesNotChangeMainView();
    void testSignalsForCreateAndRemove();
    void testUnsupportedViewTypeIsRejected();
    void testDeserializeWithoutMainViewId();
    void testDeserializeWithMissingMainView();
    void testDeserializeUnknownViewType();
    void testDeserializeDuplicateViewIds();
    void testDeserializeRestoresNameAndSettings();
    void testNullPointerHandling();
};

void TestViewManagerSerialize::testSerializeEmptyManager() {
    ViewManager manager;
    const auto dto = ViewManagerSerializer::toDTO(&manager);
    QVERIFY(dto.has_value());
    QVERIFY(dto->mainViewId.isNull());
    QVERIFY(dto->views.isEmpty());
}

void TestViewManagerSerialize::testCreateMultipleViewsAndSerializeNameAndSettings() {
    ViewManager manager;
    const auto firstId = createOpenGLView(manager);
    QVERIFY(!firstId.isNull());
    const auto secondId = createOpenGLView(manager);
    QVERIFY(!secondId.isNull());
    manager.getView(firstId)->setViewName("Primary 3D");
    manager.getView(secondId)->setViewName("Secondary 3D");
    manager.setMainView(secondId);

    const auto dto = ViewManagerSerializer::toDTO(&manager);
    QVERIFY(dto.has_value());
    QCOMPARE(dto->mainViewId, secondId);
    QCOMPARE(dto->views.size(), 2);

    const auto first = std::find_if(dto->views.cbegin(), dto->views.cend(),
                                    [firstId](const ViewDTO& view) { return view.id == firstId; });
    QVERIFY(first != dto->views.cend());
    QCOMPARE(first->name, QString("Primary 3D"));
    QCOMPARE(first->type, ViewType::OpenGL3D);
    QVERIFY(first->settings.contains("grid"));
    QVERIFY(first->settings.contains("axis"));
    QVERIFY(first->settings.contains("viewport"));
}

void TestViewManagerSerialize::testRemoveMainView() {
    ViewManager manager;
    const auto firstId = createOpenGLView(manager);
    QVERIFY(!firstId.isNull());
    const auto secondId = createOpenGLView(manager);
    QVERIFY(!secondId.isNull());
    manager.setMainView(firstId);
    manager.removeView(firstId);
    QVERIFY(!manager.getView(firstId));
    QCOMPARE(manager.getAllViews().size(), 1);
    QCOMPARE(manager.getMainViewId(), secondId);
}

void TestViewManagerSerialize::testRemoveRegularViewAndUnknownId() {
    ViewManager manager;
    const auto mainId = createOpenGLView(manager);
    QVERIFY(!mainId.isNull());
    const auto regularId = createOpenGLView(manager);
    QVERIFY(!regularId.isNull());
    QSignalSpy removedSpy(&manager, &ViewManager::viewRemoved);
    manager.removeView(regularId);
    QVERIFY(!manager.getView(regularId));
    QCOMPARE(manager.getMainViewId(), mainId);
    QCOMPARE(removedSpy.count(), 1);
    manager.removeView(QUuid::createUuid());
    QCOMPARE(manager.getAllViews().size(), 1);
    QCOMPARE(removedSpy.count(), 1);
}

void TestViewManagerSerialize::testSetUnknownViewAsMainDoesNotChangeMainView() {
    ViewManager manager;
    const auto mainId = createOpenGLView(manager);
    QVERIFY(!mainId.isNull());
    manager.setMainView(QUuid::createUuid());
    QCOMPARE(manager.getMainViewId(), mainId);
}

void TestViewManagerSerialize::testSignalsForCreateAndRemove() {
    ViewManager manager;
    QSignalSpy createdSpy(&manager, &ViewManager::viewCreated);
    QSignalSpy removedSpy(&manager, &ViewManager::viewRemoved);
    const auto id = createOpenGLView(manager);
    QVERIFY(!id.isNull());
    QCOMPARE(createdSpy.count(), 1);
    QCOMPARE(createdSpy.first().at(0).toUuid(), id);
    QCOMPARE(createdSpy.first().at(1).value<ViewType>(), ViewType::OpenGL3D);
    manager.removeView(id);
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.first().at(0).toUuid(), id);
    QCOMPARE(removedSpy.first().at(1).value<ViewType>(), ViewType::OpenGL3D);
}

void TestViewManagerSerialize::testUnsupportedViewTypeIsRejected() {
    ViewManager manager;
    QVERIFY(manager.createView(ViewType::QCustonPlot2D).isNull());
    ViewManagerDTO dto;
    dto.mainViewId = QUuid::createUuid();
    auto unsupported = makeOpenGLViewDTO(dto.mainViewId);
    unsupported.type = ViewType::QCustonPlot2D;
    dto.views.append(unsupported);
    QVERIFY(!ViewManagerSerializer::fromDTO(dto, &manager));
    QVERIFY(manager.getAllViews().isEmpty());
}

void TestViewManagerSerialize::testDeserializeWithoutMainViewId() {
    ViewManagerDTO dto;
    dto.views.append(makeOpenGLViewDTO(QUuid::createUuid()));
    ViewManager manager;
    QVERIFY(!ViewManagerSerializer::fromDTO(dto, &manager));
    QVERIFY(manager.getAllViews().isEmpty());
}

void TestViewManagerSerialize::testDeserializeWithMissingMainView() {
    ViewManagerDTO dto;
    dto.mainViewId = QUuid::createUuid();
    dto.views.append(makeOpenGLViewDTO(QUuid::createUuid()));
    ViewManager manager;
    QVERIFY(!ViewManagerSerializer::fromDTO(dto, &manager));
    QVERIFY(manager.getAllViews().isEmpty());
}

void TestViewManagerSerialize::testDeserializeUnknownViewType() {
    ViewManagerDTO dto;
    dto.mainViewId = QUuid::createUuid();
    auto view = makeOpenGLViewDTO(dto.mainViewId);
    view.type = static_cast<ViewType>(999);
    dto.views.append(view);
    ViewManager manager;
    QVERIFY(!ViewManagerSerializer::fromDTO(dto, &manager));
    QVERIFY(manager.getAllViews().isEmpty());
}

void TestViewManagerSerialize::testDeserializeDuplicateViewIds() {
    ViewManagerDTO dto;
    dto.mainViewId = QUuid::createUuid();
    dto.views.append(makeOpenGLViewDTO(dto.mainViewId, "First"));
    dto.views.append(makeOpenGLViewDTO(dto.mainViewId, "Duplicate"));
    ViewManager manager;
    QVERIFY(!ViewManagerSerializer::fromDTO(dto, &manager));
    QVERIFY(manager.getAllViews().isEmpty());
}

void TestViewManagerSerialize::testDeserializeRestoresNameAndSettings() {
    ViewManager source;
    const auto id = createOpenGLView(source);
    QVERIFY(!id.isNull());
    auto sourceView = source.getView(id);
    sourceView->setViewName("Restored 3D view");
    auto dto = ViewManagerSerializer::toDTO(&source);
    QVERIFY(dto.has_value());
    auto& viewDto = dto->views.first();
    auto gridSettings = viewDto.settings.value("grid").toMap();
    gridSettings.insert("visible", true);
    gridSettings.insert("extent", 42.0f);
    viewDto.settings.insert("grid", gridSettings);

    ViewManager restored;
    QVERIFY(ViewManagerSerializer::fromDTO(*dto, &restored));
    const auto restoredView = restored.getView(id);
    QVERIFY(restoredView);
    QCOMPARE(restored.getMainViewId(), id);
    QCOMPARE(restoredView->viewName(), QString("Restored 3D view"));
    const auto restoredGrid = restoredView->getSettingsToVariantMap().value("grid").toMap();
    QCOMPARE(restoredGrid.value("visible").toBool(), true);
    QCOMPARE(restoredGrid.value("extent").toFloat(), 42.0f);
}

void TestViewManagerSerialize::testNullPointerHandling() {
    QVERIFY(!ViewManagerSerializer::toDTO(nullptr).has_value());
    ViewManagerDTO dto;
    QVERIFY(!ViewManagerSerializer::fromDTO(dto, nullptr));
}

QTEST_MAIN(TestViewManagerSerialize)
#include "TestViewManagerSerializer.moc"