#pragma once
#include <QPointer>
#include "../AbstractView3D.h"
#include "GLViewport.h"

namespace QSpace::Visualize::Views::View3D {
// Тонкая обёртка над GLViewport, реализующая доменный контракт AbstractView3D/Views::AbstractView.
// Сам GL-код и рендер-цикл живут в GLViewport — здесь только делегирование,
// чтобы доменная логика (LayerManager и т.п.) не зависела от QOpenGLWidget напрямую.
class OpenGL3DWidget : public AbstractView3D {
    Q_OBJECT
  public:
    explicit OpenGL3DWidget(QObject* parent = nullptr);
    virtual ~OpenGL3DWidget() = default;

    QWidget* getWidget() override;
    void     render() override;
    void     setBackgroundColor(double r, double g, double b) override;

    void     setAxisVisible(bool visible) override;
    void     setGridVisible(bool visible) override;
    void     resetCamera() override;
    ViewType viewType() override;

    void                    setCameraView(View3D::CameraViewType cameraView) override;
    View3D::View3DSettings* sceneSettings() override;

    void attachRenderLayer(const QUuid&                             layerId,
                           std::shared_ptr<Visualize::IRenderLayer> layer) override;
    void detachRenderLayer(const QUuid& layerId) override;

  private:
    QPointer<GLViewport> m_viewport;
};
} // namespace QSpace::Visualize::Views::View3D