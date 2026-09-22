add_library(Views SHARED)
add_library(QSpace::VisualizeViews ALIAS Views)

set(View2D_SOURSES 

)
set(View3D_SOURSES
        View3D/AbstractView3D.h
        View3D/AbstractView3D.cpp
        View3D/View3DSettings.h
        View3D/OpenGL3DWidget/OpenGL3DWidget.h
        View3D/OpenGL3DWidget/OpenGL3DWidget.cpp
        View3D/OpenGL3DWidget/AxisRenderer.h
        View3D/OpenGL3DWidget/AxisRenderer.cpp
        View3D/OpenGL3DWidget/GridRenderer.h
        View3D/OpenGL3DWidget/GridRenderer.cpp
        View3D/OpenGL3DWidget/GLViewport.h
        View3D/OpenGL3DWidget/GLViewport.cpp
)

target_sources(Views PRIVATE
            AbstractView.h
            ViewFactory.h
            ViewFactory.cpp
            ${View3D_SOURSES}
            ${View2D_SOURSES}
)
target_include_directories(Views PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../)
target_link_libraries(Views PUBLIC
        project_config
        QSpace::Common
        QSpace::VisualizeColorMapManager
        QSpace::Session
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::OpenGL
        Qt6::OpenGLWidgets
)

