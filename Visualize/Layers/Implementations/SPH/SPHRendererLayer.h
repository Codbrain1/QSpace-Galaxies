#pragma once
#include "Common/Interfaces/IRenderLayer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QScopedPointer>
#include "Enums/LayerEnums.h"
#include "SPHPointsLayerSettings.h"
#include <memory>

namespace QSpace::Visualize::Layers {

class SPHRendererLayer : public IOpenGLRenderLayer {
  public:
    void update() override {
        m_dirty = true;
    }

    void setData(std::weak_ptr<Core::DataNode> node) override {
        m_dataNode = node;
        m_dirty    = true;
        computeBounds();
    }

    void setSettings(std::shared_ptr<LayerSettings> settings) override {
        m_settings = std::dynamic_pointer_cast<SPHPointsLayerSettings>(settings);
    }

    void setVisible(bool visible) override {
        if (m_settings) {
            m_settings->setVisible(visible);
        }
    }

    std::shared_ptr<LayerSettings> getSettings() const override {
        return m_settings;
    }

    bool isVisible() const override {
        if (m_settings) {
            return m_settings->isVisible();
        }
        return false;
    }

    void initializeGL(QOpenGLFunctions_3_3_Core* gl) override;
    void render(QOpenGLFunctions_3_3_Core* gl, const Visualize::RenderContext& ctx) override;
    void releaseGL(QOpenGLFunctions_3_3_Core* gl) override;
    bool boundingBox(QVector3D& outMin, QVector3D& outMax) const override;

    Layers::RenderLayerType type() const override {
        return RenderLayerType::SPH;
    };

  private:
    void buildShaders();
    void uploadBuffers(QOpenGLFunctions_3_3_Core* gl);
    void ensureAccumFBO(const QSize& size);
    void computeRange(QOpenGLFunctions_3_3_Core* gl);
    void computeBounds();


    QOpenGLShaderProgram m_splatProgram, m_physicalResolveProgram, m_resolveProgram;
    QOpenGLBuffer m_vboPos{QOpenGLBuffer::VertexBuffer}, m_vboScalar{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject                 m_vaoParticles;
    QOpenGLBuffer                            m_vboQuad{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject                 m_vaoQuad;
    QScopedPointer<QOpenGLFramebufferObject> m_accumFBO, m_physicalFBO;

    int  m_particleCount    = 0;
    bool m_dirty            = true;
    bool m_needsCalibration = true;

    // добавлено — нужно для boundingBox() и авто-подгонки камеры GLViewport
    QVector3D m_boundsMin{0, 0, 0};
    QVector3D m_boundsMax{0, 0, 0};
    bool      m_hasBounds = false;

    std::weak_ptr<Core::DataNode>           m_dataNode;
    std::shared_ptr<SPHPointsLayerSettings> m_settings;
};

} // namespace QSpace::Visualize::Layers