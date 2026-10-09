#include "ColorMapManagerSerializer.h"
#include "Common/Structures/SessionStructures.h"

namespace QSpace::Visualize {

QSpace::Session::ColorMapManagerDTO ColorMapManagerSerializer::toDTO() {
    QSpace::Session::ColorMapManagerDTO dto;
    dto.colorMaps = QSpace::Visualize::ColorMapManager::instance().getAllMaps();
    return dto;
}

void ColorMapManagerSerializer::fromDTO(const QSpace::Session::ColorMapManagerDTO& dto) {
    for (const auto map : dto.colorMaps) {
        QSpace::Visualize::ColorMapManager::instance().AddCustomMap(map);
    }
}

} // namespace QSpace::Visualize