add_library(ColorMapManager SHARED)
add_library(QSpace::VisualizeColorMapManager ALIAS ColorMapManager)

target_sources(ColorMapManager PRIVATE
    ColorMapManager.h
    ColorMapManager.cpp
    ColorMapTexture.h
    ColorMapTexture.cpp
)
target_include_directories(ColorMapManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(ColorMapManager PUBLIC
            project_config
            QSpace::Common
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
)