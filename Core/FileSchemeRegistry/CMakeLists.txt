add_library(FileSchemeRegistry STATIC)
target_sources(FileSchemeRegistry PRIVATE
            FileSchemeRegistry.h
            FileSchemeRegistry.cpp
)
target_include_directories(FileSchemeRegistry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(FileSchemeRegistry PUBLIC
            project_config
            QSpace::Common
) 