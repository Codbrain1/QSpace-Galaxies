#pragma once
#include "Common/Enums/ViewEnums.h"
#include "Common/Interfaces/IRenderLayer.h"
#include <quuid.h>
#include "../AbstractView.h"
#include "View3DSettings.h"
#include <memory>

namespace QSpace::Visualize::Views {
class AbstractView3D : public AbstractView {
    Q_OBJECT
  public:
    explicit AbstractView3D(QObject* parent = nullptr)
        : AbstractView(parent), m_settings(std::make_unique<View3D::View3DSettings>()) {};
    virtual ~AbstractView3D() = default;

    virtual void                    setCameraView(View3D::CameraViewType cameraView) = 0;
    virtual View3D::View3DSettings* sceneSettings()                                  = 0;

    // ---- НЕ публичный API для пользователя. Вызывается только из LayerManager. ----
    // View не решает, что рисовать — только КАК рисовать то, что ему передали.

    virtual void attachRenderLayer(const QUuid&                             layerId,
                                   std::shared_ptr<Visualize::IRenderLayer> layer) = 0;
    virtual void detachRenderLayer(const QUuid& layerId)                           = 0;

    QVariantMap getSettingsToVariantMap() const override;

    bool setSettingsFromVariantMap(const QVariantMap& settings) override;

  protected:
    std::unique_ptr<View3D::View3DSettings> m_settings;
};
} // namespace QSpace::Visualize::Views