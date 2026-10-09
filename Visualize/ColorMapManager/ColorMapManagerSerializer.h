#pragma once
#include "Common/Structures/SessionStructures.h"
#include "Visualize/ColorMapManager/ColorMapManager.h"

namespace QSpace::Visualize {

class ColorMapManagerSerializer {
  public:
    static QSpace::Session::ColorMapManagerDTO toDTO();
    static void fromDTO(const QSpace::Session::ColorMapManagerDTO& dto);
};
} // namespace QSpace::Visualize