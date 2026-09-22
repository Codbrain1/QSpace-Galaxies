#pragma once
#include "Common/Structures/SessionStructures.h"
#include "Structures/ObjectRegistryStructures.h"
#include "objectregistry_export.h"
#include <memory>


namespace QSpace::Core {
class ObjectRegistry;
}

namespace QSpace::Core {
class OBJECTREGISTRY_EXPORT ObjectRegistrySerializer {
  public:
    static std::optional<Session::ObjectRegistryDTO> toDTO(Core::ObjectRegistry* registry);
    static bool fromDTO(const Session::ObjectRegistryDTO& dto, Core::ObjectRegistry* registry);

  private:
    static std::optional<Session::ExperimentDTO> toDTO(const std::shared_ptr<Core::Experiment> experiment);
    static std::optional<Session::SnapshotDTO>   toDTO(const std::shared_ptr<Core::Snapshot> snapshot);
    static std::optional<Session::DataNodeDTO>   toDTO(const std::shared_ptr<Core::DataNode> node);
    static std::shared_ptr<Core::Experiment>     fromDTO(const Session::ExperimentDTO& dto);
    static std::shared_ptr<Core::Snapshot>       fromDTO(const Session::SnapshotDTO& dto);
    static std::shared_ptr<Core::DataNode>       fromDTO(const Session::DataNodeDTO& dto);
};
} // namespace QSpace::Core