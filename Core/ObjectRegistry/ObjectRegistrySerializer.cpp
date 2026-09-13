#include "ObjectRegistrySerializer.h"
#include "Common/Logger/Logger.h"
#include "Common/Structures/ObjectRegistryStructures.h"
#include "Common/Structures/SessionStructures.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include <qloggingcategory.h>
#include "Structures/ObjectRegistryStructures.h"
#include <memory>
#include <optional>

namespace QSpace::Core {
std::optional<Session::ObjectRegistryDTO> ObjectRegistrySerializer::toDTO(Core::ObjectRegistry* registry) {
    Session::ObjectRegistryDTO dto;
    if (!registry) {
        qCCritical(LogCore) << "ObjectRegistry is null";
        return dto;
    }

    for (const auto& experiment : registry->getAllExperiments()) {
        auto exp = toDTO(experiment);
        if (exp.has_value())
            dto.experiments.append(exp.value());
    }
    for (const auto& snapshot : registry->getAllSnapshots()) {
        auto snap = toDTO(snapshot);
        if (snap.has_value())
            dto.snapshots.append(snap.value());
    }
    for (const auto& node : registry->getAllNodes()) {
        auto dtonode = toDTO(node);
        if (dtonode.has_value())
            dto.nodes.append(dtonode.value());
    }
    return dto;
}

bool ObjectRegistrySerializer::fromDTO(const Session::ObjectRegistryDTO& dto, Core::ObjectRegistry* registry) {
    if (!registry) {
        qCCritical(LogCore) << "ObjectRegistry is null";
        return false;
    }
    registry->clear();

    QMap<QUuid, std::shared_ptr<Core::DataNode>> nodesMap;
    QMap<QUuid, std::shared_ptr<Core::Snapshot>> snapshotsMap;
    // Восстанавливаем DataNode
    for (const auto& nodeDto : dto.nodes) {
        auto node = fromDTO(nodeDto);
        if (node) {
            nodesMap.insert(node->id, node);
            registry->registerNode(node);
        }
    }
    // Восстанавливаем Snapshot и связываем их с DataNode
    for (const auto& snapshotDto : dto.snapshots) {
        auto snapshot = fromDTO(snapshotDto);
        if (snapshot) {
            // Восстанавливаем ссылки на DataNode
            for (const auto& nodeId : snapshotDto.nodes) {
                if (nodesMap.contains(nodeId)) {
                    snapshot->addComponent(nodesMap.value(nodeId));
                } else {
                    qWarning(LogSession) << "Snapshot" << snapshot->id << "references missing DataNode" << nodeId;
                }
            }
            snapshotsMap.insert(snapshot->id, snapshot);
            registry->registerSnapshot(snapshot);
        }
    }
    // Восстанавливаем Experiment и связываем их со Snapshot
    for (const auto& experimentDto : dto.experiments) {
        auto experiment = fromDTO(experimentDto);
        if (experiment) {
            // Восстанавливаем ссылки на Snapshot
            for (const auto& snapshotId : experimentDto.snapshots) {
                if (snapshotsMap.contains(snapshotId)) {
                    experiment->addSnapshot(snapshotsMap.value(snapshotId));
                } else {
                    qWarning(LogSession) << "Experiment" << experiment->id << "references missing Snapshot"
                                         << snapshotId;
                }
            }
            registry->registerExperiment(experiment);
        }
    }
    return true;
}

std::optional<Session::ExperimentDTO>
ObjectRegistrySerializer::toDTO(const std::shared_ptr<Core::Experiment> experiment) {
    if (!experiment)
        return std::nullopt;

    Session::ExperimentDTO dto;
    dto.id   = experiment->id;
    dto.name = experiment->name;
    for (const auto& snapshot : experiment->snapshots) {
        if (snapshot)
            dto.snapshots.append(snapshot->id);
    }
    return dto;
}

std::optional<Session::SnapshotDTO> ObjectRegistrySerializer::toDTO(const std::shared_ptr<Core::Snapshot> snapshot) {
    if (!snapshot)
        return std::nullopt;

    Session::SnapshotDTO dto;
    dto.id        = snapshot->id;
    dto.name      = snapshot->name;
    dto.timestamp = snapshot->timestamp;
    for (const auto& node : snapshot->components) {
        if (node)
            dto.nodes.append(node->id);
    }
    return dto;
}

std::optional<Session::DataNodeDTO> ObjectRegistrySerializer::toDTO(const std::shared_ptr<Core::DataNode> node) {
    if (!node)
        return std::nullopt;

    Session::DataNodeDTO dto;
    dto.id     = node->id;
    dto.name   = node->label;
    dto.path   = node->path;
    dto.type   = node->type;
    dto.stats  = node->stats;
    dto.format = node->format;
    dto.scheme = node->scheme;
    return dto;
}

std::shared_ptr<Core::Experiment> ObjectRegistrySerializer::fromDTO(const Session::ExperimentDTO& dto) {
    auto experiment = std::make_shared<Core::Experiment>(dto);
    return experiment;
}

std::shared_ptr<Core::Snapshot> ObjectRegistrySerializer::fromDTO(const Session::SnapshotDTO& dto) {
    auto snapshot = std::make_shared<Core::Snapshot>(dto);
    return snapshot;
}

std::shared_ptr<Core::DataNode> ObjectRegistrySerializer::fromDTO(const Session::DataNodeDTO& dto) {
    auto dataNode = std::make_shared<Core::DataNode>(dto);
    return dataNode;
}
} // namespace QSpace::Core