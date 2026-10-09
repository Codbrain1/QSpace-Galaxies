# add_subdirectory(Testing)
add_library(SessionManager STATIC)

target_sources(SessionManager PRIVATE 
SessionManager.h
SessionManager.cpp
)
target_include_directories(SessionManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(SessionManager PUBLIC
            project_config
            QSpace::CoreObjectRegistry
            QSpace::CoreLayerManager
            QSpace::CoreViewManager
            QSpace::Session
            QSpace::Visualize
            QSpace::Common
            Qt6::Core
)