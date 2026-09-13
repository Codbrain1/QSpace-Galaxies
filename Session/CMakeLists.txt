add_subdirectory(Testing)
add_library(Session STATIC)
add_library(QSpace::Session ALIAS Session)
target_sources(Session PRIVATE 
    ProjectSerializer.h
    ProjectSerializer.cpp
    SessionStorage.h
    SessionStorage.cpp
    Reflection.cpp
    Reflection.h
  
    )

target_include_directories(Session PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Session PUBLIC
            project_config
            QSpace::Common
            Qt6::Widgets
)