add_library(Interfaces STATIC)
target_sources(Interfaces PRIVATE
            IReader.h
            IWriter.h
            IRenderLayer.h
            IProjectFormatCodec.h
)
set_target_properties(Interfaces PROPERTIES AUTOMOC ON)
target_include_directories(Interfaces PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
PRIVATE  ${PROJECT_SOURCE_DIR})
target_link_libraries(Interfaces PUBLIC
            project_config
            Qt6::Core
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Enums
            
)