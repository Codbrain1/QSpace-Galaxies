include(GenerateExportHeader)
add_library(ViewManager SHARED)
add_library(QSpace::CoreViewManager ALIAS ViewManager)
generate_export_header(ViewManager)

target_sources(ViewManager PRIVATE
        ViewManager.h
        ViewManager.cpp
        ViewManagerSerializer.h
        ViewManagerSerializer.cpp
)
target_include_directories(ViewManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(ViewManager PUBLIC 
            project_config
            Qt6::Core
            QSpace::VisualizeViews
)   
add_subdirectory(Testing)