// Visualize/Layers/ParticleRendererLayer.h
#pragma once
#include "Common/Interfaces/IRenderLayer.h"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include "Enums/LayerEnums.h"
#include "ParticlePointsLayerSettings.h"

namespace QSpace::Visualize::Layers {

class ParticleRendererLayer : public IOpenGLRenderLayer {
  public:
    void update() override {
        m_dirty = true;
    }

    void setData(std::weak_ptr<Core::DataNode> node) override {
        m_dataNode = node;
        m_dirty    = true;
    }

    void setSettings(std::shared_ptr<LayerSettings> settings) override {
        m_settings = std::dynamic_pointer_cast<ParticlePointsLayerSettings>(settings);
    }

    std::shared_ptr<LayerSettings> getSettings() const override {
        return m_settings;
    }

    void setVisible(bool visible) override {
        m_visible = visible;
    }

    bool isVisible() const override {
        return m_visible;
    }

    // 1. первый шаг: подготовка контекста OpenGL, создание буферов и компиляция шейдеров
    void initializeGL(QOpenGLFunctions_3_3_Core* gl) override;
    void render(QOpenGLFunctions_3_3_Core* gl, const Visualize::RenderContext& ctx) override;
    void releaseGL(QOpenGLFunctions_3_3_Core* gl) override;
    bool boundingBox(QVector3D& outMin, QVector3D& outMax) const override;

    virtual Layers::RenderLayerType type() const override {
        return RenderLayerType::Particles;
    };

  private:
    void buildShader();
    void uploadBuffersIfDirty(QOpenGLFunctions_3_3_Core* gl);
    void computeScalarBoundsIfNeeded(const QVector<float>& scalars);

    QOpenGLShaderProgram m_program; // хранит шейдеры для GPU

    // Это обертка над VBO (Vertex Buffer Object) — областями памяти прямо в видеокарте.
    QOpenGLBuffer m_vboPosition{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_vboScalar{QOpenGLBuffer::VertexBuffer};

    // VAO — это «контейнер состояний». Вместо того чтобы каждый кадр объяснять видеокарте, в каком
    // буфере лежат координаты, а в каком скаляры, и какой у них шаг (stride), настраиваем это
    // один раз при привязке (bind) VAO. В момент отрисовки достаточно вызвать m_vao.bind(), и
    // OpenGL мгновенно вспоминает всю топологию данных.
    QOpenGLVertexArrayObject m_vao;
    int                      m_particleCount = 0;
    bool                     m_dirty         = true;
    bool                     m_visible       = true;

    std::weak_ptr<Core::DataNode>                m_dataNode;
    std::shared_ptr<ParticlePointsLayerSettings> m_settings;
};

} // namespace QSpace::Visualize::Layers