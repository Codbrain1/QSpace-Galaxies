#include "SPHRendererLayer.h"
#include "Common/Structures/ObjectRegistryStructures.h"
#include "Visualize/ColorMapManager/ColorMapTexture.h"
#include "Visualize/Layers/vtkAdapter.h"
#include "Physics/DimensionConverter/PhysicalUnits.h"
#include <algorithm>
#include <cmath>

namespace QSpace::Visualize::Layers {

void SPHRendererLayer::buildShaders() {
    m_splatProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/splat.vert");
    m_splatProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/splat.frag");

    if (!m_splatProgram.link())
        qWarning().noquote() << "SPH splat shader error:" << m_splatProgram.log();

    m_physicalResolveProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                     ":/shaders/resolve.vert");
    m_physicalResolveProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                     ":/shaders/physicalResolve.frag");
    if (!m_physicalResolveProgram.link())
        qWarning().noquote() << "SPH physical resolve shader error:"
                             << m_physicalResolveProgram.log();

    m_resolveProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/resolve.vert");
    m_resolveProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/resolve.frag");
    if (!m_resolveProgram.link())
        qWarning().noquote() << "SPH resolve shader error:" << m_resolveProgram.log();
}

void SPHRendererLayer::initializeGL(QOpenGLFunctions_3_3_Core* gl) {
    buildShaders();

    static const float quad[] = {-1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1, 1};
    m_vaoQuad.create();
    m_vaoQuad.bind();
    m_vboQuad.create();
    m_vboQuad.bind();
    m_vboQuad.allocate(quad, sizeof(quad));
    gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    gl->glEnableVertexAttribArray(0);
    m_vaoQuad.release();

    m_vaoParticles.create();
    m_vaoParticles.bind();
    m_vboPos.create();
    m_vboScalar.create();
    m_vaoParticles.release();
}

void SPHRendererLayer::computeRange(QOpenGLFunctions_3_3_Core* gl) {
    if (!m_needsCalibration || !m_physicalFBO || !m_settings)
        return;

    const int      w = m_physicalFBO->width(), h = m_physicalFBO->height();
    QVector<float> pixels(w * h * 4);

    // QOpenGLFramebufferObject::release() resets both framebuffer targets to
    // the default framebuffer. Preserve both bindings because auto-range adds
    // a readback pass between the off-screen resolve and the screen pass.
    GLint savedDrawFramebuffer = 0;
    GLint savedReadFramebuffer = 0;
    gl->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedDrawFramebuffer);
    gl->glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFramebuffer);

    m_physicalFBO->bind();
    gl->glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, pixels.data());
    m_physicalFBO->release();
    gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(savedDrawFramebuffer));
    gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(savedReadFramebuffer));

    float minimum           = std::numeric_limits<float>::max();
    float maximum           = -std::numeric_limits<float>::max();
    bool  hasValidParticles = false;

    for (int i = 0; i < w * h; ++i) {
        const float value = pixels[i * 4];
        const float Wsum  = pixels[i * 4 + 1];

        if (!std::isfinite(value) || !std::isfinite(Wsum) || Wsum < 1e-12f || value < 1e-6f)
            continue;

        minimum           = std::min(minimum, value);
        maximum           = std::max(maximum, value);
        hasValidParticles = true;
    }

    const float calculatedMinimum =
        hasValidParticles && std::isfinite(minimum) && std::isfinite(maximum) &&
                minimum <= maximum
            ? minimum
            : 0.0f;
    const float calculatedMaximum =
        hasValidParticles && std::isfinite(minimum) && std::isfinite(maximum) &&
                minimum <= maximum
            ? maximum
            : 1.0f;

    m_needsCalibration = false;
    m_settings->setCalculatedRange(calculatedMinimum, calculatedMaximum);
}

void SPHRendererLayer::ensureAccumFBO(const QSize& size) {
    const float scale = m_settings
                            ? m_settings->accumResolutionScale()
                            : 1.0f; // управляет разрешением пикселей (при уменьшении значения
                                    // понижает разрешение сетки чтобы увеличить производительность)

    // защита от ситуации когда окно свернуто
    const QSize scaled(std::max(1, int(size.width() * scale)),
                       std::max(1, int(size.height() * scale)));

    if (m_accumFBO && m_accumFBO->size() == scaled && m_physicalFBO &&
        m_physicalFBO->size() == scaled)
        return;

    QOpenGLFramebufferObjectFormat fmt;
    fmt.setInternalTextureFormat(GL_RGBA32F);
    fmt.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    m_accumFBO.reset(new QOpenGLFramebufferObject(scaled, fmt));
    m_physicalFBO.reset(new QOpenGLFramebufferObject(scaled, fmt));
    m_needsCalibration = m_settings && m_settings->autoRange();
}

void SPHRendererLayer::uploadBuffers(QOpenGLFunctions_3_3_Core* gl) {
    auto node = m_dataNode.lock();
    if (!node || !m_settings)
        return;

    QVector<QVector3D> positions = Visualize::vtkAdapter::extractPositions(node);
    QVector<float>     scalars;
    auto               normMode = SPHPointsLayerSettings::NormalizationMode::Unknown;

    if (!m_settings->colorByField().isEmpty()) {
        scalars  = Visualize::vtkAdapter::extractScalarField(node, m_settings->colorByField());
        normMode = vtkAdapter::fieldNormolized(node, m_settings->colorByField());
    }

    if (positions.isEmpty()) {
        qWarning() << "SPHRendererLayer: DataNode contains no points";
        return;
    }

    m_particleCount = positions.size();
    m_normMode      = normMode;

    auto bounds = vtkAdapter::getBounds(node);

    m_boundsMin = bounds.first;
    m_boundsMax = bounds.second;
    m_hasBounds = true;

    m_vaoParticles.bind();
    m_vboPos.bind();
    m_vboPos.allocate(positions.constData(), positions.size() * int(sizeof(QVector3D)));
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
    gl->glEnableVertexAttribArray(0);

    m_vboScalar.bind();
    if (!scalars.isEmpty()) {
        m_vboScalar.allocate(scalars.constData(), scalars.size() * int(sizeof(float)));
    } else {
        QVector<float> ones(positions.size(), 1.0f);
        m_vboScalar.allocate(ones.constData(), ones.size() * int(sizeof(float)));
    }

    gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
    gl->glEnableVertexAttribArray(1);
    m_vaoParticles.release();

    m_dirty            = false;
    m_needsCalibration = m_settings->autoRange();
}

void SPHRendererLayer::render(QOpenGLFunctions_3_3_Core* gl, const Visualize::RenderContext& ctx) {
    if (!m_settings)
        return;

    // QOpenGLFramebufferObject::release() binds framebuffer 0. For a
    // QOpenGLWidget the active render target can be a non-zero internal FBO,
    // so preserve it before the off-screen SPH passes and restore it before
    // drawing the resolved image.
    GLint renderTargetFramebuffer = 0;
    gl->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &renderTargetFramebuffer);

    // загружаем данные на видеокарту если были изменения
    if (m_dirty)
        uploadBuffers(gl);

    if (m_particleCount == 0) {
        gl->glViewport(0, 0, ctx.viewportPx.width(), ctx.viewportPx.height());
        gl->glDisable(GL_DEPTH_TEST);
        gl->glEnable(GL_BLEND);
        gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gl->glClearColor(0, 0, 0, 0);
        gl->glClear(GL_COLOR_BUFFER_BIT);
        gl->glDisable(GL_BLEND);
        return;
    }

    // увеличиваем размер сетки если необходимо
    ensureAccumFBO(ctx.viewportPx);

    // Подтягиваем астрофизический масштаб в зависимости от выбранного слоя визуализации
    // Например, если сейчас рисуется плотность, scaleFactor = l_s. Если температура = l_Te.
    auto pu = Physics::PhysicalUnits::fromSimParams(3.72, 0.9, 5.0 / 3.0, true);

    // =========================================================================
    // ПРОХОД 1: Сплаттинг (Накопление сырых сумм скаляров и количества)
    // =========================================================================
    m_accumFBO->bind();
    gl->glViewport(0, 0, m_accumFBO->width(), m_accumFBO->height());
    gl->glClearColor(0, 0, 0, 0);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    gl->glDisable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_ONE, GL_ONE);



    m_splatProgram.bind();
    m_splatProgram.setUniformValue("uMVP", ctx.mvp);
    m_splatProgram.setUniformValue("uPointScale", ctx.pixelsPerWorldUnit);
    m_splatProgram.setUniformValue("uWorldRadius", m_settings->smoothingRadius());
    // m_splatProgram.setUniformValue("uKernelType", static_cast<int>(m_settings->kernelType()));
    // масштабирование в размерные величины


    m_vaoParticles.bind();
    // непосредственная отрисовка с заданными параметрами
    gl->glDrawArrays(GL_POINTS, 0, m_particleCount);
    m_vaoParticles.release();
    m_splatProgram.release();
    gl->glDisable(GL_BLEND);
    m_accumFBO->release();

    // =========================================================================
    // ПРОХОД 2: Физический Resolve (Нормировка данных)
    // =========================================================================
    // if (m_settings->normMode() == SPHPointsLayerSettings::NormalizationMode::Intensive)

    m_physicalFBO->bind();
    gl->glViewport(0, 0, m_physicalFBO->width(), m_physicalFBO->height());
    gl->glClearColor(0, 0, 0, 0);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    gl->glDisable(GL_DEPTH_TEST);
    gl->glDisable(GL_BLEND);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, m_accumFBO->texture());


    m_physicalResolveProgram.bind();
    m_physicalResolveProgram.setUniformValue("uAccumTex", 0);
    m_physicalResolveProgram.setUniformValue("uNormalizationMode", static_cast<int>(m_normMode));

    // Отрисовываем полноэкранный прямоугольник
    m_vaoQuad.bind();
    gl->glDrawArrays(GL_TRIANGLES, 0, 6);
    m_vaoQuad.release();

    m_physicalResolveProgram.release();
    m_physicalFBO->release();


    computeRange(gl);

    // =========================================================================
    // ПРОХОД 3: Раскрашивание (Color Mapping) и вывод на экран
    // =========================================================================
    gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(renderTargetFramebuffer));
    gl->glViewport(0, 0, ctx.viewportPx.width(), ctx.viewportPx.height());
    gl->glDisable(GL_DEPTH_TEST);
    gl->glDepthMask(GL_FALSE);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, m_physicalFBO->texture());
    gl->glActiveTexture(GL_TEXTURE1);
    gl->glBindTexture(GL_TEXTURE_1D, ColorMapTexture::getOrCreate(gl, m_settings->colorMapId()));


    float scaleFactor = QSpace::Physics::fieldPhysicalScale(pu, m_settings->colorByField());

    m_resolveProgram.bind();
    m_resolveProgram.setUniformValue("uPhysicalTex", 0);
    m_resolveProgram.setUniformValue("uColorMap", 1);
    float rMin, rMax;
    if (m_settings->autoRange()) {
        rMin = m_settings->baseRangeMin();
        rMax = m_settings->baseRangeMax();
    } else {
        rMin = m_settings->rangeMin();
        rMax = m_settings->rangeMax();
    }
    m_resolveProgram.setUniformValue("uRangeMin", rMin);
    m_resolveProgram.setUniformValue("uRangeMax", rMax);
    m_resolveProgram.setUniformValue("uUseLogScale", m_settings->useLogScale());
    m_resolveProgram.setUniformValue("uGamma", m_settings->densityGamma());
    m_resolveProgram.setUniformValue("uOpacity", float(m_settings->opacity()));
    m_resolveProgram.setUniformValue("uScaleFactor", scaleFactor);


    m_vaoQuad.bind();
    gl->glDrawArrays(GL_TRIANGLES, 0, 6);
    m_vaoQuad.release();
    m_resolveProgram.release();
    gl->glActiveTexture(GL_TEXTURE1);
    gl->glBindTexture(GL_TEXTURE_1D, 0);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
    gl->glDisable(GL_BLEND);
    gl->glDepthMask(GL_TRUE);
}

void SPHRendererLayer::releaseGL(QOpenGLFunctions_3_3_Core*) {
    m_vboPos.destroy();
    m_vboScalar.destroy();
    m_vaoParticles.destroy();
    m_vboQuad.destroy();
    m_vaoQuad.destroy();
    m_accumFBO.reset();
    m_physicalFBO.reset();
}

void SPHRendererLayer::computeBounds() {
    auto data = m_dataNode.lock();
    if (!data || !data->isLoaded()) {
        return;
    }
    auto res    = vtkAdapter::getBounds(data);
    m_boundsMin = res.first;
    m_boundsMax = res.second;
    m_hasBounds = true;
}

bool SPHRendererLayer::boundingBox(QVector3D& outMin, QVector3D& outMax) const {
    if (!m_hasBounds)
        return false;
    outMin = m_boundsMin;
    outMax = m_boundsMax;
    return true;
}

} // namespace QSpace::Visualize::Layers