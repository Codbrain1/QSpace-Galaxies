find_package(Qt6 REQUIRED COMPONENTS Test Concurrent)
function(qspace_add_test TestName)
    set(test_libs ${ARGN})
    add_executable(${TestName} Cxx/${TestName}.cpp)
    
    target_link_libraries(${TestName} PRIVATE 
        Qt6::Test
        Qt6::Concurrent
        ${test_libs}    
    )

    
    add_test(NAME ${TestName} COMMAND ${TestName})
    set(QT_BIN_DIR "C:/Qt/6.11.1/mingw_64/bin")
    set(VTK_BIN_DIR "D:/NIR/NIR_6_semestr/vtk/vtk-install-ffmpeg-wmf-qt6_11-dll-OpenMP/bin")
    set_tests_properties(${TestName} PROPERTIES ENVIRONMENT 
        "PATH=${QT_BIN_DIR}\;${VTK_BIN_DIR}\;$ENV{PATH}"
    )
endfunction(qspace_add_test TestName)

function(add_sandbox_test NAME)
    # Парсим аргументы. Ожидаем списки после ключевых слов SOURCES и LIBS
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    
    # Теперь все файлы из блока SOURCES гарантированно компилируются
    add_executable(${NAME} ${ARG_SOURCES})
    
    set_target_properties(${NAME} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )
    
    # В target_link_libraries идут только таргеты из блока LIBS
    target_link_libraries(${NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${ARG_LIBS}
    )
endfunction()