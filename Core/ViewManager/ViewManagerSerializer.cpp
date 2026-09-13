#include "ViewManagerSerializer.h"
#include "Common/Logger/Logger.h"
#include "Common/Structures/SessionStructures.h"
#include "Visualize/Views/ViewFactory.h"
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

    bool allOk = true;

    for (const auto& viewDto : dto.views) {
        auto view = fromDTO(viewDto);
        if (view) {
            viewManager->addView(view);
        } else {
            allOk = false;
            qCWarning(LogCore) << "view: " << viewDto.id << " is not deserialized";
        }
    }

    if (viewManager->getView(dto.mainViewId)) { // если такой метод есть, либо getView(id) != nullptr
        viewManager->setMainView(dto.mainViewId);
    } else {
        allOk = false;
        qCWarning(LogCore) << "Main view" << dto.mainViewId << "was not restored, main view not set";
    }

    return allOk;
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