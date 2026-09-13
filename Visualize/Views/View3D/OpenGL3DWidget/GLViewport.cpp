#include "GLViewport.h"
#include "Common/Logger/Logger.h"
#include "Visualize/ColorMapManager/ColorMapManager.h"
#include "Visualize/ColorMapManager/ColorMapTexture.h"
#include <QMouseEvent>
#include <QPainter>
#include <QQuaternion>
#include <QWheelEvent>
#include <QtMath>
#include <qelapsedtimer.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include "Enums/ViewEnums.h"
#include "Physics/DimensionConverter/PhysicalUnits.h"
#include <algorithm>
#include <cmath>

namespace QSpace::Visualize::Views::View3D {

GLViewport::GLViewport(View3DSettings* settings, QWidget* parent)
    : QOpenGLWidget(parent), m_settings(settings) {
    if (!m_settings) {
        qCCritical(LogRenderer()) << "Settings in GLViewPort is not initialized!";
        return;
    }

    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(m_settings->viewport()->multisamples());
    setFormat(fmt);

    // держим виджет в курсе изменений настроек сцены (grid/axis/viewport) —
    // перерисовка при любом изменении
    connect(m_settings, &View3DSettings::anyChanged, this, [this]() {
        m_backgroundColor = m_settings->viewport()->backgroundColor();
        update();
    });

    // ---- палитры: любое изменение состава/содержимого палитр в ColorMapManager
    // может сделать закэшированные GPU-текстуры устаревшими. Сам вызов
    // ColorMapTexture::invalidate() здесь НЕЛЬЗЯ делать напрямую — слот вызывается
    // без гарантированно активного GL-контекста этого виджета. Поэтому только
    // помечаем нужный id как "грязный" и просим перерисовку; реальная инвалидация
    // происходит в paintGL(), где makeCurrent() уже выполнен фреймворком Qt.
    connect(&ColorMapManager::instance(),
            &ColorMapManager::paleteAdded,
            this,
            [this](const ColorMap& map) {
                m_pendingColorMapInvalidations.insert(map.id);
                update();
            });

    m_backgroundColor = m_settings->viewport()->backgroundColor();

    setMouseTracking(true);
}

GLViewport::~GLViewport() {
    makeCurrent();
    for (auto& layer : m_layers)
        layer->releaseGL(this);
    m_gridRenderer.releaseGL(this);
    m_axisRenderer.releaseGL(this);
    Layers::ColorMapTexture::releaseForContext(context());
    doneCurrent();
}

void GLViewport::attachRenderLayer(const QUuid&                                   layerId,
                                   std::shared_ptr<Visualize::IOpenGLRenderLayer> layer) {
    if (!layer)
        return;

    if (isValid()) {
        makeCurrent();
        layer->initializeGL(this);
        doneCurrent();
    }
    m_layers.insert(layerId, layer);
    m_needsCameraFit = true;
    update();
}

void GLViewport::detachRenderLayer(const QUuid& layerId) {
    auto it = m_layers.find(layerId);
    if (it == m_layers.end())
        return;

    if (isValid()) {
        makeCurrent();
        it.value()->releaseGL(this);
        doneCurrent();
    }
    m_layers.erase(it);
    update();
}

void GLViewport::setBackgroundColor(double r, double g, double b) {
    m_backgroundColor = QColor::fromRgbF(r, g, b);
    m_settings->viewport()->setBackgroundColor(m_backgroundColor); // держим настройки синхронными
    update();
}

void GLViewport::resetCamera() {
    m_yaw            = 0.0f;
    m_pitch          = 0.3f; // небольшой наклон по умолчанию — удобнее чистого top-down
    m_needsCameraFit = true;
    update();
}

void GLViewport::setCameraPreset(CameraViewType presetIndex) {
    switch (presetIndex) {
        case CameraViewType::XY_Top:
            m_yaw   = 0.0f;
            m_pitch = qDegreesToRadians(89.9f);
            break; // Top
        case CameraViewType::XZ_Front:
            m_yaw   = 0.0f;
            m_pitch = 0.0f;
            break; // Front
        case CameraViewType::YZ_Right:
            m_yaw   = qDegreesToRadians(90.0f);
            m_pitch = 0.0f;
            break; // Side
        default:
            m_yaw   = qDegreesToRadians(35.0f);
            m_pitch = qDegreesToRadians(25.0f);
            break; // Isometric
    }
    update();
}

void GLViewport::forceFullRedraw() {
    m_needsCameraFit = true;
    update();
}

void GLViewport::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glClearColor(float(m_backgroundColor.redF()),
                 float(m_backgroundColor.greenF()),
                 float(m_backgroundColor.blueF()),
                 1.0f);

    m_gridRenderer.initializeGL(this);
    m_axisRenderer.initializeGL(this);

    for (auto& layer : m_layers)
        layer->initializeGL(this);
}

void GLViewport::resizeGL(int, int) {
}

void GLViewport::fitCameraToLayers() {
    QVector3D globalMin, globalMax;
    bool      any = false;

    for (auto& layer : m_layers) {
        QVector3D lo, hi;
        if (layer->boundingBox(lo, hi)) {
            if (!any) {
                globalMin = lo;
                globalMax = hi;
                any       = true;
            } else {
                globalMin.setX(std::min(globalMin.x(), lo.x()));
                globalMin.setY(std::min(globalMin.y(), lo.y()));
                globalMin.setZ(std::min(globalMin.z(), lo.z()));
                globalMax.setX(std::max(globalMax.x(), hi.x()));
                globalMax.setY(std::max(globalMax.y(), hi.y()));
                globalMax.setZ(std::max(globalMax.z(), hi.z()));
            }
        }
    }

    if (!any)
        return;

    m_center         = (globalMin + globalMax) * 0.5f;
    const float diag = (globalMax - globalMin).length();
    m_distance       = diag > 0.0f ? diag * 0.75f : 10.0f;
    m_axisExtent     = diag > 0.0f ? diag * 0.5f : 10.0f;
    m_settings->grid()->setExtent(m_axisExtent);
}

Visualize::RenderContext GLViewport::buildRenderContext() {
    Visualize::RenderContext ctx;
    ctx.viewportPx   = size(); // получаем размер окна (для виджета отрисовки непосредственно)
    ctx.orthographic = m_settings->viewport()->orthographic();

    const float aspect = width() > 0 ? float(width()) / float(std::max(1, height()))
                                     : 1.0f; // вычисляем соотношение сторон чтобы при растягивании
                                             // окна не искажалась геометрия объектов


    if (ctx.orthographic) { // все частицы проецируются на экран путем параллельных лучей
                            // (оргогональная проеция)
        const float halfH = std::max(m_distance, 0.001f);
        const float halfW = halfH * aspect;
        ctx.projMatrix.ortho(-halfW,
                             halfW,
                             -halfH,
                             halfH,
                             m_settings->viewport()->nearClip(),
                             m_settings->viewport()->farClip());
        ctx.pixelsPerWorldUnit = float(height()) / (2.0f * halfH);
    } else { // все частицы проецируются на экран
        ctx.projMatrix.perspective(m_settings->viewport()->fovYDegrees(),
                                   aspect,
                                   m_settings->viewport()->nearClip(),
                                   m_settings->viewport()->farClip());
        const float fovYRad = qDegreesToRadians(m_settings->viewport()->fovYDegrees());
        float       visibleWorldHeightAtDistance = 2.0f * m_distance * std::tan(fovYRad * 0.5f);
        ctx.pixelsPerWorldUnit = float(height()) / std::max(visibleWorldHeightAtDistance, 0.001f);
    }

    ctx.cameraPos = QVector3D(m_center.x() + m_distance * std::cos(m_pitch) * std::sin(m_yaw),
                              m_center.y() + m_distance * std::sin(m_pitch),
                              m_center.z() + m_distance * std::cos(m_pitch) * std::cos(m_yaw));

    QVector3D forward      = (m_center - ctx.cameraPos).normalized();
    bool      isUpsideDown = std::cos(m_pitch) < 0.0f;
    QVector3D baseUp       = isUpsideDown ? QVector3D(0, -1, 0) : QVector3D(0, 1, 0);

    // Используем m_roll (в радианах)
    QQuaternion rotation = QQuaternion::fromAxisAndAngle(forward, qRadiansToDegrees(m_roll));
    QVector3D   rolledUp = rotation.rotatedVector(baseUp);

    ctx.viewMatrix.lookAt(ctx.cameraPos, m_center, rolledUp);


    ctx.mvp = ctx.projMatrix * ctx.viewMatrix;

    return ctx;
}

void GLViewport::processPendingColorMapInvalidations() {
    // вызывается ИЗНУТРИ paintGL() — здесь GL-контекст этого виджета точно активен
    // (Qt гарантирует makeCurrent() перед вызовом paintGL()), поэтому здесь и только
    // здесь безопасно удалять GL-текстуры палитр.
    if (m_pendingColorMapInvalidations.isEmpty())
        return;

    for (const QUuid& id : std::as_const(m_pendingColorMapInvalidations))
        Layers::ColorMapTexture::invalidate(id);

    m_pendingColorMapInvalidations.clear();
}

void GLViewport::paintGL() {
    processPendingColorMapInvalidations();
    if (m_needsCameraFit) {
        fitCameraToLayers();
        m_needsCameraFit = false;
    }

    Visualize::RenderContext ctx = buildRenderContext();
    m_lastContext                = ctx;

    glClearColor(float(m_backgroundColor.redF()),
                 float(m_backgroundColor.greenF()),
                 float(m_backgroundColor.blueF()),
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_gridRenderer.render(this, ctx, m_settings->grid());

    for (auto it = m_layers.constBegin(); it != m_layers.constEnd(); ++it) {
        if (!it.value()->isVisible())
            continue;

        it.value()->render(this, ctx);

        // уведомляем внешние подписчики (colorbar) о том, что у этого слоя
        // могли поменяться визуальные параметры (диапазон авто-калибруется
        // некоторыми рендерерами прямо внутри render(), например SPHRendererLayer) —
        // colorbar сам решает, нужно ли ему перечитать rangeMin/rangeMax/colorMapId
        emit layerVisualsChanged(it.key());
    }
    m_axisRenderer.render(this, ctx, m_settings->axis(), m_axisExtent);
}

void GLViewport::paintEvent(QPaintEvent* event) {
    QOpenGLWidget::paintEvent(event); // выполнит paintGL() через внутренний механизм Qt

    if (!m_settings->axis()->visible())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(
        QFont(m_settings->axis()->labelFontFamily(), m_settings->axis()->labelFontSize()));

    const auto ticks = m_axisRenderer.buildTicks(m_settings->axis(), m_axisExtent);
    for (const auto& tick : ticks) {
        const QVector4D clip = m_lastContext.mvp * QVector4D(tick.worldPos, 1.0f);
        if (clip.w() <= 0.0f)
            continue;

        const QPointF screen((clip.x() / clip.w() * 0.5f + 0.5f) * width(),
                             (1.0f - (clip.y() / clip.w() * 0.5f + 0.5f)) * height());

        painter.setPen(tick.color);
        painter.drawText(screen, tick.text);
    }
}

void GLViewport::mousePressEvent(QMouseEvent* event) {
    m_dragging     = true;
    m_lastMousePos = event->pos();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging)
        return;

    const QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos     = event->pos();
    QVector3D right    = m_lastContext.viewMatrix.row(0).toVector3D();
    QVector3D up       = m_lastContext.viewMatrix.row(1).toVector3D();

    if (event->modifiers() & Qt::ShiftModifier) {
        // --- ПАНОРАМИРОВАНИЕ (PAN) ---
        float scale = (m_lastContext.pixelsPerWorldUnit > 0.0f)
                          ? (1.0f / m_lastContext.pixelsPerWorldUnit)
                          : 0.01f;


        m_center += (-right * delta.x() + up * delta.y()) * scale;

    } else if (event->modifiers() & Qt::ControlModifier) {
        // --- ВРАЩЕНИЕ (ROLL) ---
        m_roll = normalizeAngle(m_roll + delta.x() * 0.01f);
    } else {
        // --- ОРБИТА (ORBIT) ---
        float yawDelta = delta.x() * 0.01f;

        // Если локальный вектор "вверх" (up) камеры смотрит вниз относительно
        // мировой оси Y (up.y() < 0), значит камера перевернута.
        // Неважно, произошло это из-за pitch или roll — просто инвертируем yaw.
        if (up.y() < 0.0f) {
            yawDelta = -yawDelta;
        }

        m_yaw   = normalizeAngle(m_yaw - yawDelta);
        m_pitch = normalizeAngle(m_pitch + delta.y() * 0.01f);
    }

    update();
}

void GLViewport::mouseReleaseEvent(QMouseEvent*) {
    m_dragging = false;
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    const float delta = event->angleDelta().y() / 120.0f;
    m_distance        = std::max(m_distance * std::pow(0.9f, delta), 0.001f);
    update();
}

} // namespace QSpace::Visualize::Views::View3D