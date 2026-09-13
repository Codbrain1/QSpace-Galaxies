add_library(AppCore STATIC)
target_sources(AppCore PRIVATE 
            AppCore.h
            AppCore.cpp
            ViewController.h
            ViewController.cpp
            DataController.h
            DataController.cpp
            ProjectController.h
            ProjectController.cpp
            VideoController.h
            VideoController.cpp
)
target_include_directories(AppCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(AppCore PUBLIC 
                project_config
                QSpace::Common
                QSpace::IO
                QSpace::Visualize
                QSpace::Session
                QSpace::Models
                ObjectRegistry
                DataManager
                TaskManager
                SessionManager
                FileSchemeRegistry
                QSpace::Physics
                Qt6::Core
)