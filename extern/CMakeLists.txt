add_library(Extern INTERFACE)
add_library(QSpace::Extern ALIAS Extern)
target_include_directories(Extern INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Extern INTERFACE project_config)