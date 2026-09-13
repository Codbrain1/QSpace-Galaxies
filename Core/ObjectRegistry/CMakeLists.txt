add_subdirectory(Testing)
add_library(ObjectRegistry SHARED)
target_sources(ObjectRegistry PRIVATE
            ObjectRegistry.h
            ObjectRegistry.cpp
            ObjectRegistrySerializer.h
            ObjectRegistrySerializer.cpp
)
target_include_directories(ObjectRegistry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(ObjectRegistry PUBLIC
            project_config
            Qt6::Core
            QSpace::Physics
            VTK::CommonDataModel
)