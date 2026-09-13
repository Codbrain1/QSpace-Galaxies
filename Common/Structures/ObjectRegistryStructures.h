#pragma once
#include "Common/Enums/IOEnums.h"
#include "Common/Enums/VisualizeBaseEnums.h"
#include <QMap>
#include <QObject>
#include <QPair>
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <quuid.h>
#include <vtkDataSet.h>
#include <vtkSmartPointer.h>
#include "DataNodeMetaData.h"
#include "SessionStructures.h"
#include <memory>



class vtkDataSet;

namespace QSpace::Core {

// CRITICAL: полностью избавится от vtk
struct DataNode {
    Q_GADGET

    Q_PROPERTY(QUuid id MEMBER id)
    Q_PROPERTY(QString label MEMBER label)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(QSpace::Visualize::EntityType type MEMBER type)
    Q_PROPERTY(QSpace::IO::FileFormat format MEMBER format)
    Q_PROPERTY(QUuid scheme MEMBER scheme)

  public:
    QUuid                       id;     // уникальный идентификатор для связи между слоями и данными
    QString                     label;  // метка для отображения данных в UI
    QString                     path;   // путь к файлу с данными
    Visualize::EntityType       type;   // тип данных для быстрой подстройки визуализации
    vtkSmartPointer<vtkDataSet> data;   // непосредственно данные
    QSpace::IO::FileFormat      format; // формат файла (бинарный, текстовый и т.д.)
    QUuid                       scheme; // схема для чтения данных, нужна для их восстановления)

    DataNodeMetaData stats;

    DataNode(vtkSmartPointer<vtkDataSet> dataSet,
             const QString&              name,
             double                      timestamp = 0.0,
             Visualize::EntityType       t         = Visualize::EntityType::Unknown)
        : id(QUuid::createUuid()), label(name), type(t), data(dataSet) {
        stats.timestamp = timestamp;
    }

    // // Version 1.0
    DataNode(const QSpace::Session::DataNodeDTO& dto)
        : id(dto.id),
          label(dto.name),
          path(dto.path),
          type(dto.type),
          format(dto.format),
          scheme(dto.scheme),
          stats(dto.stats) {
    }

    inline bool isLoaded() {
        return data ? true : false;
    }
};

class Snapshot {
    Q_GADGET
    Q_PROPERTY(QUuid id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(double timestamp MEMBER timestamp)


  public:
    QUuid                            id;
    QString                          name;
    double                           timestamp;
    QList<std::shared_ptr<DataNode>> components;

    Snapshot(const QString& snapshotName, double ts = 0.0)
        : id(QUuid::createUuid()), name(snapshotName), timestamp(ts) {
    }

    Snapshot(const QSpace::Session::SnapshotDTO& dto)
        : id(dto.id), name(dto.name), timestamp(dto.timestamp), components() {
    }

    void addComponent(std::shared_ptr<DataNode> node) {
        if (node) {
            components.append(node);
        }
    }

    bool isLoaded() {
        for (const auto& node : components) {
            if (node->isLoaded())
                return true;
        }
        return false;
    }
};

struct Experiment {
    Q_GADGET
  public:
    QUuid                            id;
    QString                          name;
    QList<std::shared_ptr<Snapshot>> snapshots; // Список временных шагов

    Experiment(const QString& expName) : id(QUuid::createUuid()), name(expName) {
    }

    Experiment(const QSpace::Session::ExperimentDTO& dto) : id(dto.id), name(dto.name) {
    }

    void addSnapshot(std::shared_ptr<Snapshot> snapshot) {
        if (snapshot)
            snapshots.append(snapshot);
    }
};

} // namespace QSpace::Core