#pragma once
#include "Common/Structures/ObjectRegistryStructures.h"
#include "Common/Structures/RenderContext.h"
#include "Visualize/Layers/LayerSettings.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QVector3D>
#include <memory>

namespace QSpace::Visualize
{

// Базовый интерфейс для любой отрисовки
class IRenderLayer
{
public:
  virtual ~IRenderLayer() = default;
  virtual void update() = 0;
  virtual void setData(std::weak_ptr<Core::DataNode> node) = 0;
  virtual std::shared_ptr<Layers::LayerSettings> getSettings() const = 0;
  virtual void setSettings(std::shared_ptr<Layers::LayerSettings> settings) = 0;
  virtual void setVisible(bool visible) = 0;
  virtual bool isVisible() const = 0;
  virtual Layers::RenderLayerType type() const = 0;
};

// Интерфейс исключительно для VTK
// TODO deprecated, удалить после перехода на OpenGL
// class IVtkRenderLayer : public IRenderLayer
// {
// public:
//   virtual vtkSmartPointer<vtkProp> getVtkProp() = 0;
//   virtual vtkSmartPointer<vtkScalarBarActor> getScalarBar() const = 0;
//   virtual void attachInteractor(vtkRenderWindowInteractor *interactor) = 0;
//   virtual void detachInteractor() = 0;
//   virtual void updateColorsForContrast(double contrast) = 0;
// };
// Интерфейс исключительно для OpenGL — новая ветка, зеркало IVtkRenderLayer
class IOpenGLRenderLayer : public IRenderLayer
{
public:
  // GLViewport вызывает это один раз при создании GL-контекста (или при attach,
  // если контекст уже создан) — здесь слой создаёт свои буферы/шейдеры
  virtual void initializeGL(QOpenGLFunctions_3_3_Core *gl) = 0;

  // основной проход отрисовки, вызывается GLViewport каждый кадр для видимых слоёв
  virtual void render(QOpenGLFunctions_3_3_Core *gl, const Visualize::RenderContext &ctx) = 0;

  // освобождение GPU-ресурсов перед уничтожением контекста/detach
  virtual void releaseGL(QOpenGLFunctions_3_3_Core *gl) = 0;

  // для автоподгонки камеры под данные слоя; false — если слой не может
  // предоставить границы (например, ещё нет данных)
  virtual bool boundingBox(QVector3D &outMin, QVector3D &outMax) const = 0;
};
} // namespace QSpace::Visualize