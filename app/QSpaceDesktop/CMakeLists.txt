# 1. Настройка переводов
#------------------------------------------
set(TS_FILES
    ${PROJECT_SOURCE_DIR}/app/translates/QSpace_ru.ts
)
#добавление файлов перевода
qt6_add_translations(
    QSpace
    TS_FILES ${TS_FILES}
RESOURCE_PREFIX "/i18n"
)

#создание исполнимого qt файла
qt_add_executable(QSpace
    MANUAL_FINALIZATION
    main.cpp
    ${PROJECT_SOURCE_DIR}/app/resources/icons.qrc
    ${PROJECT_SOURCE_DIR}/app/resources/styles.qrc
    ${PROJECT_SOURCE_DIR}/app/resources/shaders.qrc
    ${QM_FILES}
)

target_link_libraries(QSpace PRIVATE
            project_config
            QSpace::Common
            QSpace::Core
            QSpace::IO
            QSpace::Visualize
            QSpace::Models
            # Charting
            # Filtering
            # Physics
            # Remote
            # QSpace::Session
            QSpace::UI
            qcustomplot
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            # Qt6::Quick
            # Qt6::Qml
            VTK::GUISupportQt
            VTK::CommonColor
            VTK::ImagingHybrid 
            VTK::RenderingVolumeOpenGL2
            VTK::RenderingOpenGL2
            VTK::InteractionStyle
            OpenMP::OpenMP_CXX
)
# Генерация макросов инициализации фабрик
vtk_module_autoinit(
    TARGETS QSpace
    MODULES ${VTK_LIBRARIES}
)

if(WIN32) # на виндовс только gui
    set_target_properties(QSpace PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_sources(QSpace PRIVATE ${PROJECT_SOURCE_DIR}/app/resources/app_icon.rc)
    set(VTK_BIN_DIR "D:/NIR/NIR_6_semestr/vtk/vtk-install-ffmpeg-wmf-qt6_11-dll-OpenMP/bin")

    add_custom_command(TARGET QSpace POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
    "${VTK_BIN_DIR}" 
    "$<TARGET_FILE_DIR:QSpace>"
    COMMENT "Copying VTK binaries..."
    )
    add_custom_command(TARGET QSpace POST_BUILD
    COMMAND "C:/Qt/6.11.1/mingw_64/bin/windeployqt.exe"
    --compiler-runtime
        $<TARGET_FILE:QSpace>
            COMMENT "Running windeployqt..."
            )
endif()
        
include(GNUInstallDirs)
install(TARGETS QSpace
BUNDLE DESTINATION .
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/" 
DESTINATION bin 
FILES_MATCHING 
PATTERN "*.dll"        # Забираем библиотеки
PATTERN "platforms/*"  # Забираем критический плагин qwindows.dll
PATTERN "styles/*"
PATTERN "imageformats/*"
)
qt_finalize_executable(QSpace)