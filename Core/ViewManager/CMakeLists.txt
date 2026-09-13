add_library(ViewManager SHARED)
target_sources(ViewManager PRIVATE
        ViewManager.h
        ViewManager.cpp
        ViewManagerSerializer.h
        ViewManagerSerializer.cpp
)
target_include_directories(ViewManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(ViewManager PUBLIC 
            project_config
            Qt6::Core
            QSpace::VisualizeViews
)   