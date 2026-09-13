add_subdirectory(LogManager)
add_subdirectory(DataManager)
add_subdirectory(TaskManager)
# add_subdirectory(PipelineManager)
add_subdirectory(ObjectRegistry)
add_subdirectory(ViewManager)
add_subdirectory(LayerManager)
# add_subdirectory(VideoExportManager)
add_subdirectory(AppCore)
add_subdirectory(SessionManager)
add_subdirectory(StyleManager)
add_subdirectory(FileSchemeRegistry)

add_library(Core INTERFACE)
add_library(QSpace::Core ALIAS Core)
target_include_directories(Core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Core INTERFACE
            project_config
            TaskManager
            ObjectRegistry
            DataManager
            LogManager
            LayerManager
            AppCore
            ViewManager
            StyleManager
            # VideoExportManager
            FileSchemeRegistry
            SessionManager
            Qt6::Core
)