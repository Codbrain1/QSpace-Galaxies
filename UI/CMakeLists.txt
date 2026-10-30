set(CMAKE_AUTOUIC ON)
add_library(UI STATIC)
add_library(QSpace::UI ALIAS UI)
target_sources(UI PRIVATE
    Home/MainWindow.h
    Home/MainWindow.cpp
    Home/ToastNotification.h
    Home/ToastNotification.cpp
    Home/newmainwindow.ui
    Home/LayerExplorerWidget.h
    Home/LayerExplorerWidget.cpp
    Home/PropertyInspector.h
    Home/PropertyInspector.cpp
    Home/PropertyInspector.ui
    Home/ColorMapComboBox.h
    Home/ColorMapComboBox.cpp
    Home/ColorMapEditorDialog.h
    Home/ColorMapEditorDialog.cpp
    Home/ColorMapEditorDialog.ui
    Home/TimeLineWidget.h
    Home/TimeLineWidget.cpp
    Home/TimeLineWidget.ui
    Home/SelectExperimentDialog.h
    Home/SelectExperimentDialog.cpp
    Home/SettingsEditorWidget.h
    Home/SettingsEditorWidget.cpp
    Home/ColorbarWidget.h
    Home/ColorbarWidget.cpp
    Home/ViewContainerWidget.h
    Home/ViewContainerWidget.cpp
    Home/AutoSettingsToolBar.h
    Home/AutoSettingsToolBar.cpp
    Home/PropertyControlFactory.h
    Home/PropertyControlFactory.cpp

)
target_include_directories(UI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(UI PUBLIC
    project_config
    QSpace::Core
    QSpace::Visualize
    QSpace::Physics
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    VTK::GUISupportQt
    qcustomplot
)