add_subdirectory(ColorMapManager)
add_subdirectory(Layers)
add_subdirectory(Views)

add_library(Visualize INTERFACE)
add_library(QSpace::Visualize ALIAS Visualize)
target_include_directories(Visualize INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Visualize INTERFACE
            project_config
            QSpace::VisualizeViews       
            QSpace::VisualizeLayers
            QSpace::VisualizeColorMapManager
)