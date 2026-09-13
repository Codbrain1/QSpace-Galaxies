#pragma once
#include "Common/Structures/SessionStructures.h"

namespace QSpace::Core {
class FileSchemeRegistry;
}

namespace QSpace::Session {
class FileSchemeRegistrySerializer {
  public:
    static FileSchemeRegistryDTO toDTO(Core::FileSchemeRegistry* registry);
    bool fromDTO(const FileSchemeRegistryDTO& dto, Core::FileSchemeRegistry* registry);
};
} // namespace QSpace::Session