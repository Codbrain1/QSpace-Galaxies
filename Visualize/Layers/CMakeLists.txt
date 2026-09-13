
set(BinningLayer 
Implementations/Binning/BinningPointsLayerSettings.h
Implementations/Binning/BinningRendererLayer.cpp
Implementations/Binning/BinningRendererLayer.h
)

set(ParticleLayer 
Implementations/Particle/ParticleRendererLayer.h
Implementations/Particle/ParticleRendererLayer.cpp
Implementations/Particle/ParticlePointsLayerSettings.h
)

set(SPHLayer 
Implementations/SPH/SPHRendererLayer.h
Implementations/SPH/SPHRendererLayer.cpp
Implementations/SPH/SPHPointsLayerSettings.h
)

add_library(Layers SHARED)
add_library(QSpace::VisualizeLayers ALIAS Layers) 

target_sources(Layers PRIVATE
           Layer.h
           LayerSettings.h
           LayerFactory.h
           LayerFactory.cpp
           vtkAdapter.h
            ${BinningLayer}
            ${ParticleLayer}
            ${SPHLayer}
)
target_include_directories(Layers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(Layers PUBLIC
            project_config
            QSpace::Common
            QSpace::Physics
            Qt6::Core
            Qt6::OpenGL
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            VTK::CommonCore
)