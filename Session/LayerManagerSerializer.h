#pragma once
#include "Common/Structures/SessionStructures.h"
#include "Visualize/Layers/Layer.h"
#include "Visualize/Layers/LayerSettings.h"
#include <memory>

namespace QSpace::Core {
class LayerManager;
}

namespace QSpace::Session {
class LayerManagerSerializer {
  public:
    static LayerManagerDTO toDTO(Core::LayerManager* layerManager);
    bool                   fromDTO(const LayerManagerDTO& dto, Core::LayerManager* layerManager);

  private:
    static LayerSettingsDTO
                    toDTO(const std::shared_ptr<Visualize::Layers::LayerSettings> layerSettings);
    static LayerDTO toDTO(const std::shared_ptr<Visualize::Layers::Layer> layer);

    static std::shared_ptr<Visualize::Layers::LayerSettings> fromDTO(const LayerSettingsDTO& dto);
    static std::shared_ptr<Visualize::Layers::Layer>         fromDTO(const LayerDTO& dto);
};
} // namespace QSpace::Session