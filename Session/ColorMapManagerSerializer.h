#pragma once
#include "Common/Structures/SessionStructures.h"
#include "Visualize/ColorMapManager/ColorMapManager.h"

namespace QSpace::Visualize {
class ColorMapManager;
}

namespace QSpace::Session {
class ColorMapManagerSerializer {
  public:
    static ColorMapManagerDTO toDTO(Visualize::ColorMapManager* colorMapManager);
    bool fromDTO(const ColorMapManagerDTO& dto, Visualize::ColorMapManager* colorMapManager);
};
} // namespace QSpace::Session