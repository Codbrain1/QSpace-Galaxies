cmake_minimum_required(VERSION 3.16)
project(qcustomplot_module VERSION 2.1.1)

find_package(Qt6 6.11 COMPONENTS Core Gui Widgets PrintSupport REQUIRED)

message(STATUS "=======================================================")
message(STATUS " Target Qt Version: 6.11")
message(STATUS " Detected Qt Version: ${Qt6_VERSION}")
message(STATUS " Qt Installation Dir: ${Qt6_DIR}")
message(STATUS "=======================================================")

set(CMAKE_AUTOMOC ON)

add_library(qcustomplot STATIC 
    qcustomplot.cpp 
    qcustomplot.h
)

set_target_properties(qcustomplot PROPERTIES 
    AUTOMOC ON
)

target_compile_options(qcustomplot PRIVATE "-fno-lto")

target_include_directories(qcustomplot SYSTEM PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(qcustomplot PRIVATE 
        "-Wa,-mbig-obj"                 # Разрешаем большие объектные файлы (убирает Fatal Error)
        "-Wno-deprecated-declarations"  # Скрываем предупреждения об устаревшем коде Qt 6.11
    )
endif()

target_link_libraries(qcustomplot PUBLIC 
    Qt6::Widgets 
    Qt6::PrintSupport
)