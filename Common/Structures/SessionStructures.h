#pragma once
#include "Common/Enums/IOEnums.h"
#include "Common/Enums/LayerEnums.h"
#include "Common/Enums/ViewEnums.h"
#include "Common/Enums/VisualizeBaseEnums.h"
#include "Common/Structures/FileSchemeStructures.h"
#include "Visualize/Layers/LayerSettings.h"
#include <QColor>
#include <QList>
#include <QMap>
#include <qlist.h>
#include <qsharedpointer.h>
#include <qtmetamacros.h>
#include <quuid.h>
#include <qvariant.h>
#include "DataNodeMetaData.h"
#include "FileSchemeRegistry.h"
#include "Structures/ColormapPresets.h"
#include <float.h>

namespace QSpace::Session {
Q_NAMESPACE

enum class ProjectFormat {
    Json,
    Xml
};
Q_ENUM_NS(ProjectFormat)

class ExperimentDTO {
    Q_GADGET
    Q_PROPERTY(QUuid id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QList<QUuid> snapshots MEMBER snapshots)
  public:
    QUuid        id;
    QString      name;
    QList<QUuid> snapshots;

    bool operator==(const ExperimentDTO& other) const = default;
};

class SnapshotDTO {
    Q_GADGET
    Q_PROPERTY(QUuid id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(double timestamp MEMBER timestamp)
    Q_PROPERTY(QList<QUuid> nodes MEMBER nodes)
  public:
    QUuid        id;
    QString      name;
    double       timestamp;
    QList<QUuid> nodes;

    bool operator==(const SnapshotDTO& other) const = default;
};

class DataNodeDTO {
    Q_GADGET
    Q_PROPERTY(QUuid id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(Visualize::EntityType type MEMBER type)
    Q_PROPERTY(QSpace::Core::DataNodeMetaData stats MEMBER stats)
    Q_PROPERTY(IO::FileFormat format MEMBER format)

  public:
    QUuid                          id;
    QString                        name;
    QString                        path;
    Visualize::EntityType          type;
    QSpace::Core::DataNodeMetaData stats;
    IO::FileFormat                 format;
    QUuid scheme; // обрабатывается через отдельные методы потому что это variant
    bool  operator==(const DataNodeDTO&) const = default;
};

class ObjectRegistryDTO {
    Q_GADGET
    Q_PROPERTY(QList<ExperimentDTO> experiments MEMBER experiments)
    Q_PROPERTY(QList<SnapshotDTO> snapshots MEMBER snapshots)
    Q_PROPERTY(QList<DataNodeDTO> nodes MEMBER nodes)
  public:
    QList<ExperimentDTO> experiments;
    QList<SnapshotDTO>   snapshots;
    QList<DataNodeDTO>   nodes;
};

class ViewDTO {
    Q_GADGET
    Q_PROPERTY(QUuid id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(Visualize::Views::ViewType type MEMBER type)
    Q_PROPERTY(QVariantMap settings MEMBER settings)

  public:
    QUuid                      id;
    QString                    name;
    Visualize::Views::ViewType type;
    QVariantMap                settings; // сериализуемый вариант настроек конкретного типа View
    bool                       operator==(const ViewDTO&) const = default;
};

class ViewManagerDTO {
    Q_GADGET
    Q_PROPERTY(QUuid mainViewId MEMBER mainViewId)
    Q_PROPERTY(QList<ViewDTO> views MEMBER views)
  public:
    QUuid          mainViewId;
    QList<ViewDTO> views;
};

class LayerDTO {
    Q_GADGET
    Q_PROPERTY(QUuid layerId MEMBER layerId)
    Q_PROPERTY(QUuid nodeId MEMBER nodeId)
    Q_PROPERTY(QUuid viewId MEMBER viewId)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(bool isSynced MEMBER isSynced)
    Q_PROPERTY(QSpace::Visualize::Layers::RenderLayerType renderType MEMBER renderType)
    Q_PROPERTY(QVariantMap settings MEMBER settings)
  public:
    QUuid                              layerId;
    QUuid                              nodeId;   // На какой DataNode ссылается
    QUuid                              viewId;   // В каком окне отрисовывается
    QString                            name;     // Имя слоя (может отличаться от имени DataNode)
    bool                               isSynced; // Синхронизирован ли слой с мастер-
    Visualize::Layers::RenderLayerType renderType;
    QVariantMap settings; // сериализуемый вариант настроек конкретного типа View
    bool        operator==(const LayerDTO&) const = default;
};

class LayerManagerDTO {
    Q_GADGET
    Q_PROPERTY(QList<LayerDTO> layers MEMBER layers)
  public:
    QList<LayerDTO> layers;
};

class FileSchemeRegistryDTO {
    Q_GADGET
    Q_PROPERTY(QList<QSpace::IO::ReadSchemeUI> readSchemes MEMBER readSchemes)
    Q_PROPERTY(QList<QSpace::IO::WriteSchemeUI> writeSchemes MEMBER writeSchemes)
  public:
    QList<IO::ReadSchemeUI>  readSchemes;
    QList<IO::WriteSchemeUI> writeSchemes;
};

class ColorMapManagerDTO {
    Q_GADGET
    Q_PROPERTY(QList<QSpace::Visualize::ColorMap> colorMaps MEMBER colorMaps)
  public:
    QList<Visualize::ColorMap> colorMaps;
};

class ProjectDTO {
    Q_GADGET
    Q_PROPERTY(QSpace::Session::ObjectRegistryDTO objregDTO MEMBER objregDTO)
    Q_PROPERTY(QSpace::Session::ViewManagerDTO viewmanDTO MEMBER viewmanDTO)
    Q_PROPERTY(QSpace::Session::LayerManagerDTO layermanDTO MEMBER layermanDTO)
    Q_PROPERTY(QSpace::Session::ColorMapManagerDTO colormapmanDTO MEMBER colormapmanDTO)

  public:
    ObjectRegistryDTO  objregDTO;
    ViewManagerDTO     viewmanDTO;
    LayerManagerDTO    layermanDTO;
    ColorMapManagerDTO colormapmanDTO;
};

struct [[deprecated("Use DataNodeDTO instead")]] DataNodeState {
    std::shared_ptr<Visualize::Layers::LayerSettings> settings;
    QUuid                                             id;
    QString                                           label;
    QString                                           path;
    Visualize::EntityType                             type;
    QSpace::Core::DataNodeMetaData                    stats;
    IO::FileFormat                                    format;
    IO::ReadScheme                                    scheme;
};

struct [[deprecated("Use LayerDTO instead")]] LayerState {
    QUuid                                             layerId;
    QUuid                                             nodeId;   // На какой DataNode ссылается
    QUuid                                             viewId;   // В каком окне отрисовывается
    std::shared_ptr<Visualize::Layers::LayerSettings> settings; // Настройки ИМЕННО ЭТОГО слоя
};

class [[deprecated("Use SnapshotDTO instead")]] SnapshotState { // TODO:: может оказаться излишним
};

struct [[deprecated("Use ProjectDTO instead")]] ProjectState {
    QString              version = "1.0";
    QString              projectName;
    QList<DataNodeState> nodesStates;
    QList<LayerState>    layersStates;
};

struct [[deprecated("Don't use this structure")]] CurrentSession {
    QString projectName;
    QString projectFilePath;
    bool    isDirty = false;
};
} // namespace QSpace::Session

Q_DECLARE_METATYPE(QSpace::Session::ExperimentDTO)
Q_DECLARE_METATYPE(QSpace::Session::SnapshotDTO)
Q_DECLARE_METATYPE(QSpace::Session::DataNodeDTO)
Q_DECLARE_METATYPE(QSpace::Session::ObjectRegistryDTO)
Q_DECLARE_METATYPE(QSpace::Session::ViewDTO)
Q_DECLARE_METATYPE(QSpace::Session::ViewManagerDTO)
Q_DECLARE_METATYPE(QSpace::Session::LayerDTO)
Q_DECLARE_METATYPE(QSpace::Session::LayerManagerDTO)
Q_DECLARE_METATYPE(QSpace::Session::FileSchemeRegistryDTO)
Q_DECLARE_METATYPE(QSpace::Session::ColorMapManagerDTO)
Q_DECLARE_METATYPE(QSpace::Session::ProjectDTO)
Q_DECLARE_METATYPE(QSpace::Session::ProjectFormat)