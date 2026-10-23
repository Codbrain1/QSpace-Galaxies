#pragma once
#include "Common/Enums/VisualizeBaseEnums.h"
#include "Visualize/Views/AbstractView.h"
#include <QObject>
#include <QStringList>
#include <quuid.h>
#include <memory>

namespace QSpace::Core {
class ViewManager;
class LayerManager;
class ObjectRegistry;
// class VideoExportManager;
} // namespace QSpace::Core

namespace QSpace::Visualize::Layers {
class Layer;
}

namespace QSpace::Visualize::Views {
class AbstractView;
}

namespace QSpace::Core::Controllers {
// ---------------------------------------------------------
// @SECTION: Экспорт видео
// ---------------------------------------------------------
class VideoController : public QObject {
    Q_OBJECT
  public:
    explicit VideoController(Core::ObjectRegistry* objectRegistry,
                             Core::ViewManager*    viewManager,
                             Core::LayerManager*   layerManager,
                             QObject*              parent = nullptr);

    double getSnapshotTimeByIndex(int index) {
        if (index > 0 && index < static_cast<int>(m_timeSliderSnapshots.size()))
            return m_timeSliderSnapshots[index].timestamp;
        else
            return 0;
    };

    QUuid getCurrentSnapshotId() const {
        return m_currentVisualizeSnapshotId;
    }

    void initialize();

  signals:
    void snapshotsListSizeChanged(const int size);
    void requestNodeLoad(const QUuid& id);
    void changedEtalonLayer(const QList<QUuid>& layers);

  public slots:
    // применяем эксперимент для отображения
    void handleTargetExperimentChange(const QUuid& experimentId);
    // активируем отображаемый снимок
    void handleActivateVisualizeSnapshot(int index);

    // повторно обновляет слой после завершения асинхронной загрузки его данных
    void handleNodeDataLoaded(const QUuid& nodeId);

    // зафиксировать ТЕКУЩИЙ кадр как новый эталон
    void handleFixedEtalonSnapshot(const QUuid& referenceSnapshotId);

  private:
    struct SnapshotToTimestamp {
        QUuid  targetSnapshot;
        double timestamp;

        bool operator<(const SnapshotToTimestamp& other) const {
            return timestamp < other.timestamp;
        }
    };

    void requestLoadFloatWindow(int index);

    const int floatWindow = 15;
    QMap<QSpace::Visualize::EntityType,
         QList<std::shared_ptr<QSpace::Visualize::Layers::Layer>>>
        m_snapshotPlayerSate; // слои для текущего snapshot

    QUuid m_currentVisualizeSnapshotId; // текущий отображаемый кадр
    // QUuid m_referenceSnapshotId; // "эталонный" снимок, на который зафиксирован диапазон окраски
    QUuid m_targetExperimentId; // эксперимент отображаемый с помощью плеера кадров
    std::vector<SnapshotToTimestamp> m_timeSliderSnapshots;


    Core::ObjectRegistry* m_objectRegistry;
    Core::ViewManager*    m_viewManager;
    Core::LayerManager*   m_layerManager;
};

} // namespace QSpace::Core::Controllers