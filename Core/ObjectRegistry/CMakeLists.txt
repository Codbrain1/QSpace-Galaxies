include(GenerateExportHeader)
add_library(ObjectRegistry SHARED)
add_library(QSpace::CoreObjectRegistry ALIAS ObjectRegistry)
generate_export_header(ObjectRegistry)

target_sources(ObjectRegistry PRIVATE
ObjectRegistry.h
ObjectRegistry.cpp
ObjectRegistrySerializer.h
ObjectRegistrySerializer.cpp
)
target_include_directories(ObjectRegistry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(ObjectRegistry PUBLIC
project_config
Qt6::Core
QSpace::Physics
VTK::CommonDataModel
)
add_subdirectory(Testing)