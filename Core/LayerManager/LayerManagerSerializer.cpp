#pragma once
#include "LayerManagerSerializer.h"
#include "Common/Structures/SessionStructures.h"
#include "Visualize/Layers/Layer.h"
#include "Visualize/Layers/LayerFactory.h"
#include <quuid.h>
#include "LayerManager.h"
#include "Session/Reflection.h"
#include <memory>
#include <optional>

#include "Session/Reflection.h"

namespace QSpace::Core {

std::optional<Session::LayerManagerDTO> LayerManagerSerializer::toDTO(Core::LayerManager* layerManager) {
    if (!layerManager)
        return std::nullopt;

    Session::LayerManagerDTO dto;

    auto layersList = layerManager->getAllLayers();
    for (auto& layer : layersList) {
        if (layer) {
            auto layer_dto = toDTO(layer);
            if (layer_dto.has_value()) {
                dto.layers.append(layer_dto.value());
            }
        }
    }
    return dto;
}

bool LayerManagerSerializer::fromDTO(const Session::LayerManagerDTO& dto,
                                     Core::LayerManager*             layerManager,
                                     NodeResolver                    nodeResolver,
                                     ViewResolver                    viewResolver) {
    if (!layerManager) {
        return false;
    }

    bool allSucceeded = true;

    for (const auto& layerDTO : dto.layers) {
        auto layer = fromDTO(layerDTO, nodeResolver, viewResolver);
        if (layer) {
            layerManager->registerLayer(layer);
        } else {
            allSucceeded = false;
        }
    }

    return allSucceeded;
}

std::optional<Session::LayerDTO> LayerManagerSerializer::toDTO(const std::shared_ptr<Visualize::Layers::Layer> layer) {
    if (!layer)
        return std::nullopt;

    Session::LayerDTO dto;
    dto.layerId = layer->layerId();
    dto.nodeId  = layer->dataNodeId();
    if (auto view = layer->getView().lock()) {
        dto.viewId = view->id();
    } else {
        dto.viewId = QUuid();
    }
    dto.name     = layer->name();
    dto.isSynced = layer->IsSyncedWithMaster();
    if (auto engine = layer->getEngine()) {
        dto.renderType = engine->type();
    }
    if (auto settings = layer->getSettings()) {
        dto.settings = Reflection::QObjectToVariantMap(settings.get());
    }
    return dto;
}

std::shared_ptr<Visualize::Layers::Layer>
LayerManagerSerializer::fromDTO(const Session::LayerDTO& dto, NodeResolver nodeResolver, ViewResolver viewResolver) {
    // 1. Находим зависимости по ID
    auto node = nodeResolver ? nodeResolver(dto.nodeId) : nullptr;
    auto view = viewResolver ? viewResolver(dto.viewId) : nullptr;

    if (!node || !view) {
        qWarning() << "LayerManagerSerializer::fromDTO: Node or View not found for layer" << dto.layerId;
        return nullptr;
    }

    // 2. Создаем базовый объект Layer
    auto layer = std::make_shared<Visualize::Layers::Layer>(node, view);

    // ВАЖНО: устанавливаем сохраненный ID до assignEngine!
    layer->setLayerId(dto.layerId);
    layer->setName(dto.name);
    layer->setIsSyncedWithMaster(dto.isSynced);

    // 3. Создаем движок рендеринга через фабрику с типом из DTO
    auto renderEngine = Visualize::Layers::LayerFactory::createLayerRenderer(node, dto.renderType);
    if (!renderEngine) {
        qWarning() << "LayerManagerSerializer::fromDTO: Failed to create render engine for layer" << dto.layerId;
        return nullptr;
    }

    // 4. Привязываем движок (он же зарегистрирует слой во View3D с правильным layerId)
    layer->assignEngine(renderEngine);

    // 5. Восстанавливаем настройки (LayerSettings) через Reflection
    if (auto settings = layer->getSettings()) {
        Reflection::variantMapToQObject(dto.settings, settings.get());
    }

    // 6. Обновляем слой для пересчета буферов и триггера перерисовки
    layer->update();

    return layer;
}

} // namespace QSpace::Core