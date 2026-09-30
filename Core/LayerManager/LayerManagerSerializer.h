#pragma once
#include "Common/Structures/SessionStructures.h"
#include "Visualize/Layers/Layer.h"
#include "Visualize/Layers/LayerSettings.h"
#include "layermanager_export.h"
#include <memory>

namespace QSpace::Core {
class LayerManager;
}

namespace QSpace::Core {
class LAYERMANAGER_EXPORT LayerManagerSerializer {
  public:
    using NodeResolver = std::function<std::shared_ptr<DataNode>(const QUuid&)>;
    using ViewResolver = std::function<std::shared_ptr<Visualize::Views::AbstractView>(const QUuid&)>;

    static std::optional<Session::LayerManagerDTO> toDTO(Core::LayerManager* layerManager);
    static bool                                    fromDTO(const Session::LayerManagerDTO& dto,
                                                           Core::LayerManager*             layerManager,
                                                           NodeResolver                    nodeResolver,
                                                           ViewResolver                    viewResolver);

    static std::optional<Session::LayerDTO> toDTO(const std::shared_ptr<Visualize::Layers::Layer> layer);
    static std::shared_ptr<Visualize::Layers::Layer>
    fromDTO(const Session::LayerDTO& dto, NodeResolver nodeResolver, ViewResolver viewResolver);
};
} // namespace QSpace::Core