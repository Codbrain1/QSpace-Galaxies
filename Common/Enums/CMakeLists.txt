add_library(Enums INTERFACE)
target_include_directories(Enums INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Enums INTERFACE 
        project_config
)