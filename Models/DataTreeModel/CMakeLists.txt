if(ENABLE_TESTS)
    add_subdirectory(Testing)
endif()

add_library(DataTreeModel STATIC)
target_sources(DataTreeModel PRIVATE
            DataTreeModel.h
            DataTreeModel.cpp
)
target_include_directories(DataTreeModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(DataTreeModel PUBLIC
            project_config
            QSpace::Common
            QSpace::Core
            Qt6::Core
) 