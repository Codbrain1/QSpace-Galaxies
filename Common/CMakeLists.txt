

add_subdirectory(Enums)
add_subdirectory(Interfaces)
add_subdirectory(Logger)
# add_subdirectory(PhysicalConstants)
add_subdirectory(Structures)
# add_subdirectory(Version)


add_library(CommonMetaObjects STATIC)
target_sources(CommonMetaObjects PRIVATE
    Enums/IOEnums.h
    Enums/CoreEnums.h
    Enums/VisualizeBaseEnums.h

    Structures/ColormapPresets.h
    Structures/FileSchemeStructures.h
    Structures/FileSchemeRegistry.h
    Structures/ObjectRegistryStructures.h
    Structures/SessionStructures.h
)
set_target_properties(CommonMetaObjects PROPERTIES
    AUTOMOC ON
)
target_include_directories(CommonMetaObjects PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(CommonMetaObjects PUBLIC
    project_config
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    VTK::CommonCore
    VTK::CommonDataModel
)

add_library(QSpace::CommonMetaObjects ALIAS CommonMetaObjects)

add_library(Common INTERFACE)
add_library(QSpace::Common ALIAS Common)
target_include_directories(Common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Common INTERFACE
    Enums
    Interfaces
    Logger
    # PhysicalConstants
    Structures
    # Version
    CommonMetaObjects
    project_config
    )

