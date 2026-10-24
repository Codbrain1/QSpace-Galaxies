cmake_minimum_required(VERSION 3.16) #минимальная версия cmake

# 1. основные сведедения о проекте
#-------------------------------------------------------------------
project(QSpace VERSION 1.0 LANGUAGES CXX) # название проекта его версия и язык на котором написан проект
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()
set(CMAKE_CXX_STANDARD 20) # стандарт языка 23
set(CMAKE_CXX_STANDARD_REQUIRED ON)#обязательный стандарт языка
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(ENABLE_TESTS "Build Tests" ON) #опция переключения между тестами

# 2. Настройка окружения проекта (Compiler)

#=========== Настройки для виндовс ==============
if(WIN32) 
    set(CMAKE_PREFIX_PATH 
    "C:/Qt/6.11.1/mingw_64"
    "D:/NIR/NIR_6_semestr/vtk/vtk-install-ffmpeg-wmf-qt6_11-dll-OpenMP/lib/cmake/vtk-9.5")
    # Отключение Vulkan
    set(QT_NO_VULKAN ON)
    set(CMAKE_CXX_COMPILER "C:/Qt/Tools/mingw1310_64/bin/g++.exe")
    set(CMAKE_C_COMPILER "C:/Qt/Tools/mingw1310_64/bin/gcc.exe")
endif()


find_package(OpenMP REQUIRED)
# 3. поиск зависимостей
# Core - базовые классы
# Gui - графика
# Widgets - различные виджеты(окна, кнопки и т.д.)
# LinguistTools - модуль с переводами
find_package(Qt6 REQUIRED COMPONENTS 
            # Quick
            # Qml
            Core 
            Gui 
            Widgets 
            LinguistTools
            )
find_package(VTK REQUIRED COMPONENTS 
            CommonCore 
            CommonDataModel 
            CommonColor
            IOCore
            IOOggTheora
            IOVideo
            GUISupportQt
            ImagingHybrid
            RenderingVolumeOpenGL2
            RenderingCore             # Обязательно
            RenderingOpenGL2          # Обязательно
            InteractionStyle
            RenderingVolume          # Абстрактный интерфейс объема
            RenderingVolumeOpenGL2   # Реализация GPU-рейкастинга
            FiltersPoints
            )

# автоматическая генерация для qt
#------------------------------------
set(CMAKE_AUTOMOC ON) # создает moc_*.cpp из .h c Q_Object
set(CMAKE_AUTORCC ON) #встраивает qrc ресурсы
set(CMAKE_AUTOUIC ON) #обработка .ui файлов
set(QT_NO_QML_PLUGINS ON) #отключаем qml плагины
set(QT6_NO_IMPORT_PLUGINS ON)
set(QT_NO_VIRTUALKEYBOARD ON)
#------------------------------------

# 4. флаги компиляции 
#--------------------------------------
add_library(project_config INTERFACE)
target_compile_options(project_config INTERFACE -Wall -Wextra -fopenmp)
target_compile_options(project_config INTERFACE $<$<CONFIG:Release>:-O2>)
target_compile_options(project_config INTERFACE $<$<CONFIG:Debug>: -O0 -g3 -fno-omit-frame-pointer>)
target_compile_definitions(project_config INTERFACE
    $<$<CONFIG:Debug>:QSPACE_PROJECT_DEBUG_MODE>
)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

target_precompile_headers(project_config INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/pch.h"
)
# 5. исполняемые файлы помещают в bin, библиотеки в lib
#------------------------------------------
set(OUTPUT_SUBDIR $<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${OUTPUT_SUBDIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${OUTPUT_SUBDIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${OUTPUT_SUBDIR}/lib)

if(ENABLE_TESTS)
    message("-- Test Enabled")
    enable_testing()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Tests.cmake)
endif()

add_subdirectory(app/QSpaceDesktop)
add_subdirectory(Common)
add_subdirectory(Core)
# add_subdirectory(Filtering)
add_subdirectory(IO)
add_subdirectory(Models)
add_subdirectory(Physics)
# add_subdirectory(Remote)
add_subdirectory(Visualize)
add_subdirectory(Session)
add_subdirectory(UI)
# add_subdirectory(Charting) [[deprecated]]
add_subdirectory(Sandbox)
add_subdirectory(extern/QCustomPlot)

include(GNUInstallDirs)
set(CPACK_PACKAGE_NAME "QSpace")
set(CPACK_PACKAGE_VENDOR "Shorkin")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "QSpace")
# --- Настройки отображения в Windows (Установка и удаление программ) ---
set(CPACK_NSIS_DISPLAY_NAME "QSpace Visualization Tool")
set(CPACK_NSIS_CONTACT "PRIb-231_126178@volsu.ru") # Добавляет контактную информацию
# set(CPACK_NSIS_URL_INFO_ABOUT "https://твой-сайт.com") # Ссылка на проект
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\QSpace.exe") # Иконка для Панели управления

# --- Кастомизация дизайна установщика (MUI2) ---
# ВАЖНО: Пути должны быть абсолютными. Используем ${CMAKE_SOURCE_DIR}
# NSIS принимает только формат .ico для иконок и .bmp для картинок!
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/app/resources/icons/logo.ico")
# set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/app/icons/uninstall_icon.ico")
# Картинка слева на приветственном экране (рекомендуемый размер 164x314)
# set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/app/icons/wizard_side.bmp")
# Картинка в верхней панели (рекомендуемый размер 150x57)
# set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_SOURCE_DIR}/app/icons/wizard_header.bmp")

# --- Создание ярлыков ---
# Добавляем программу в меню "Пуск"
# Формат: "имя_исполняемого_файла_без_exe;Название_Ярлыка"
set(CPACK_PACKAGE_EXECUTABLES "QSpace;QSpace")

# Команды для создания ярлыка на рабочем столе (выполняются при установке)
# Обрати внимание на экранирование слешей \\ для NSIS
set(CPACK_NSIS_CREATE_ICONS_EXTRA "
  CreateShortCut '$DESKTOP\\\\QSpace.lnk' '$INSTDIR\\\\bin\\\\QSpace.exe'
")

# Команды для удаления ярлыка с рабочего стола (выполняются при удалении)
set(CPACK_NSIS_DELETE_ICONS_EXTRA "
  Delete '$DESKTOP\\\\QSpace.lnk'
")
include(CPack)
