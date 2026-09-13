#include "Common/Structures/SessionStructures.h"
#include "Core/ViewManager/ViewManager.h"

namespace QSpace::Core {
class ViewManager;
}

namespace QSpace::Core {
class ViewManagerSerializer {
  public:
    static std::optional<Session::ViewManagerDTO> toDTO(Core::ViewManager* viewManager);
    static bool fromDTO(const Session::ViewManagerDTO& dto, Core::ViewManager* viewManager);

  private:
    static std::optional<Session::ViewDTO> toDTO(const std::shared_ptr<Visualize::Views::AbstractView> view);
    static std::shared_ptr<Visualize::Views::AbstractView> fromDTO(const Session::ViewDTO& dto);
};
} // namespace QSpace::Core