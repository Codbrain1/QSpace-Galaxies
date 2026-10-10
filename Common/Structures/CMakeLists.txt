add_library(Structures INTERFACE)
target_include_directories(Structures INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(Structures INTERFACE
                VTK::CommonCore
                VTK::CommonDataModel
                Qt6::Core
                Qt6::Concurrent
                project_config
                )
