#include "ViewManagerSerializer.h"
#include "Common/Logger/Logger.h"
#include "Common/Structures/SessionStructures.h"
#include "Visualize/Views/ViewFactory.h"
#include <QSet>
#include <qloggingcategory.h>
#include "ViewManager.h"
#include <optional>


namespace QSpace::Core {

std::optional<Session::ViewManagerDTO> ViewManagerSerializer::toDTO(Core::ViewManager* viewManager) {
    if (!viewManager) {
        qCritical(LogCore) << "viewManager is null";
        return std::nullopt;
    }

    Session::ViewManagerDTO dto;
    dto.mainViewId = viewManager->getMainViewId();
    for (const auto& view : viewManager->getAllViews()) {
        auto dtoView = toDTO(view);
        if (dtoView.has_value())
            dto.views.append(dtoView.value());
    }
    return dto;
}

bool ViewManagerSerializer::fromDTO(const Session::ViewManagerDTO& dto, Core::ViewManager* viewManager) {
    if (!viewManager) {
        qCritical(LogCore) << "viewManager is null";
        return false;
    }

    if (dto.views.isEmpty()) {
        if (!dto.mainViewId.isNull()) {
            qCWarning(LogCore) << "Main view" << dto.mainViewId << "is missing from an empty DTO";
            return false;
        }
        viewManager->clear();
        return true;
    }

    if (dto.mainViewId.isNull()) {
        qCWarning(LogCore) << "View DTO does not define a main view";
        return false;
    }

    QSet<QUuid> viewIds;
    for (const auto& viewDto : dto.views) {
        if (viewDto.id.isNull() || viewIds.contains(viewDto.id)) {
            qCWarning(LogCore) << "View DTO contains a null or duplicate ID" << viewDto.id;
            return false;
        }
        viewIds.insert(viewDto.id);
    }
    if (!viewIds.contains(dto.mainViewId)) {
        qCWarning(LogCore) << "Main view" << dto.mainViewId << "is missing from DTO";
        return false;
    }

    QList<std::shared_ptr<Visualize::Views::AbstractView>> restoredViews;
    restoredViews.reserve(dto.views.size());
    for (const auto& viewDto : dto.views) {
        auto view = fromDTO(viewDto);
        if (!view) {
            qCWarning(LogCore) << "View" << viewDto.id << "is not deserialized";
            return false;
        }
        restoredViews.append(std::move(view));
    }

    viewManager->clear();
    for (const auto& view : restoredViews) {
        if (!viewManager->addView(view)) {
            qCWarning(LogCore) << "View" << view->id() << "could not be added";
            return false;
        }
    }

    viewManager->setMainView(dto.mainViewId);
    return true;
}

std::optional<Session::ViewDTO>
ViewManagerSerializer::toDTO(const std::shared_ptr<Visualize::Views::AbstractView> view) {
    if (!view)
        return std::nullopt;

    Session::ViewDTO dto;

    dto.id       = view->id();
    dto.name     = view->viewName();
    dto.type     = view->viewType();
    dto.settings = view->getSettingsToVariantMap();
    return dto;
}

std::shared_ptr<Visualize::Views::AbstractView> ViewManagerSerializer::fromDTO(const Session::ViewDTO& dto) {
    auto view = Visualize::Views::ViewFactory::createView(dto.type);
    if (!view)
        return nullptr;
    view->setId(dto.id);
    view->setViewName(dto.name);
    if (!view->setSettingsFromVariantMap(dto.settings)) {
        qCWarning(LogCore) << "Failed to apply settings for view" << dto.id << "- using defaults";
    }
    return view;
}
} // namespace QSpace::Core