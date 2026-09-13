#include "ProjectSerializer.h"
#include "Common/Enums/VisualizeBaseEnums.h"
#include "Visualize/ColorMapManager/ColorMapManager.h"
#include <QVariant>
#include <qloggingcategory.h>
#include "Enums/IOEnums.h"
#include "Logger/Logger.h"
#include <memory>

namespace QSpace::Session {

// =========================================================================
// ОСНОВНЫЕ МЕТОДЫ ПРОЕКТА
// =========================================================================

QByteArray ProjectSerializer::serialize(const QSpace::Session::ProjectState& project_state) {
    QJsonObject root;
    root.insert("version", project_state.version);

    // 1. СОХРАНЯЕМ КАСТОМНЫЕ ПАЛИТРЫ ГЛОБАЛЬНО
    QJsonArray palettesArr;
    for (const auto& map : Visualize::ColorMapManager::instance().getAllMaps()) {
        if (!map.isPreset) { // Пресеты не сохраняем, они жестко вшиты в код
            palettesArr.append(serializeColorMap(map));
        }
    }
    root.insert("customColorMaps", palettesArr);

    // 2. СОХРАНЯЕМ НОДЫ
    QJsonArray arr;
    for (const auto& node_state : project_state.nodesStates) {
        // Теперь мы просто вызываем выделенный метод
        arr.append(serializeDataNode(node_state));
    }
    root.insert("nodes", arr);

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented);
}

std::optional<ProjectState> ProjectSerializer::deserialize(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return std::nullopt;
    }

    QJsonObject  root = doc.object();
    ProjectState state;
    state.version = root["version"].toString("1.0");

    // 1. СНАЧАЛА ЗАГРУЖАЕМ ПАЛИТРЫ В МЕНЕДЖЕР
    if (root.contains("customColorMaps")) {
        QJsonArray palettesArr = root["customColorMaps"].toArray();
        for (auto v : palettesArr) {
            auto map = deserializeColorMap(v.toObject());
            // Регистрируем, чтобы ноды могли их найти при загрузке ниже
            Visualize::ColorMapManager::instance().AddCustomMap(map);
        }
    }

    // 2. ЗАТЕМ ЗАГРУЖАЕМ НОДЫ
    QJsonArray nodesArr = root["nodes"].toArray();
    for (const auto& nodeVal : nodesArr) {
        if (nodeVal.isObject()) {
            state.nodesStates.append(deserializeDataNode(nodeVal.toObject()));
        }
    }

    return state;
}

// =========================================================================
// СЕРИАЛИЗАЦИЯ НОДЫ (ИСПРАВЛЕНА ОШИБКА)
// =========================================================================

QJsonObject ProjectSerializer::serializeDataNode(const QSpace::Session::DataNodeState& node_state) {
    QJsonObject o;
    o.insert("id", node_state.id.toString());
    o.insert("label", node_state.label);
    o.insert("path", node_state.path);
    o.insert("format",
             IO::fileformatToString(
                 node_state.format)); // Проверь: ...ToString или ...String в твоих енамах
    o.insert("entityType", QSpace::Visualize::entitytypeToString(node_state.type));
    // Bounds
    QJsonArray boundsArr;
    for (int i = 0; i < 6; ++i)
        boundsArr.append(node_state.stats.bounds[i]);
    o.insert("bounds", boundsArr);

    // Counts (приводим к qint64 явно, чтобы избежать предупреждений)
    o.insert("cellCount", static_cast<qint64>(node_state.stats.cellCount));
    o.insert("pointCount", static_cast<qint64>(node_state.stats.pointCount));

    // CenterOfMass
    QJsonArray centerOfMassArr;
    for (int i = 0; i < 3; ++i)
        centerOfMassArr.append(node_state.stats.centerOfMass[i]);
    o.insert("centerOfMass", centerOfMassArr);

    // ScalarRanges
    QJsonObject rangesObj;
    for (const auto& [key, range] : node_state.stats.scalarRanges.asKeyValueRange()) {
        QJsonObject r;
        r.insert("min", range.first);
        r.insert("max", range.second);
        rangesObj.insert(key, r);
    }
    o.insert("scalarRanges", rangesObj);

    // Вложенные структуры
    // o.insert("visualSettings", serializeVisualSettings(node_state.settings));
    o.insert("readScheme", serializeReadScheme(node_state.scheme));

    return o;
}

DataNodeState ProjectSerializer::deserializeDataNode(const QJsonObject& json) {
    DataNodeState node;

    node.id    = QUuid::fromString(json["id"].toString());
    node.label = json["label"].toString();
    node.path  = json["path"].toString();

    node.type   = Visualize::entitytypeFromString(json["entityType"].toString());
    node.format = IO::fileformatFromString(json["format"].toString());

    QJsonArray bArr = json["bounds"].toArray();
    for (int i = 0; i < 6 && i < bArr.size(); ++i) {
        node.stats.bounds[i] = bArr[i].toDouble();
    }

    node.stats.cellCount  = json["cellCount"].toVariant().toLongLong();
    node.stats.pointCount = json["pointCount"].toVariant().toLongLong();

    QJsonArray comArr = json["centerOfMass"].toArray();
    for (int i = 0; i < 3 && i < comArr.size(); ++i) {
        node.stats.centerOfMass[i] = comArr[i].toDouble();
    }

    QJsonObject rangesObj = json["scalarRanges"].toObject();
    for (auto it = rangesObj.begin(); it != rangesObj.end(); ++it) {
        QJsonObject r = it.value().toObject();
        node.stats.scalarRanges.insert(it.key(), {r["min"].toDouble(), r["max"].toDouble()});
    }

    // if (json.contains("visualSettings")) {
    //     node.settings = deserializeVisualSettings(json["visualSettings"].toObject());
    // }
    if (json.contains("readScheme")) {
        node.scheme = deserializeReadScheme(json["readScheme"].toObject());
    }

    return node;
}

// =========================================================================
// ВИЗУАЛЬНЫЕ НАСТРОЙКИ
// =========================================================================

QJsonObject ProjectSerializer::serializeVisualSettings(
    const QSpace::Visualize::Layers::LayerSettings& settings) {
    QJsonObject obj;
    // obj.insert("mode", QSpace::Visualize::rendermodeToString(settings.mode));
    // obj.insert("colorMapId", settings.colorMapId.toString());
    // obj.insert("isVisible", settings.isVisible);
    // obj.insert("useLogScale", settings.useLogScale);
    // obj.insert("showScalarBar", settings.showScalarBar);
    // obj.insert("autoRange", settings.autoRange);
    // obj.insert("pointSize", settings.PointSize);
    // obj.insert("opacity", settings.opacity);
    // obj.insert("interpolationRangeType", settings.interpolationRangeType);
    // obj.insert("shaderType", settings.ShaderType);
    // obj.insert("interpolationOpacityFunction", settings.interpolationOpacityFunction);
    // obj.insert("gaussianSharpness", settings.gaussianSharpness);
    // obj.insert("sigmoidGammaOpacity", settings.sigmoidGammaOpacity);
    // obj.insert("sigmoidShiftOpacity", settings.sigmoidShiftOpacity);
    // obj.insert("sigmoidGammaColor", settings.sigmoidGammaColor);
    // obj.insert("sigmoidShiftColor", settings.sigmoidShiftColor);
    // obj.insert("alpha", settings.alpha);
    // // obj.insert("beta", settings.beta);
    // obj.insert("rangeMin", settings.rangeMin);
    // obj.insert("rangeMax", settings.rangeMax);
    // obj.insert("colorByField", settings.colorByField);
    // obj.insert("isEmisive", settings.isEmmisive);
    // obj.insert("exposureClamp", settings.exposureClamp);
    // obj.insert("baseRangeMin", settings.baseRangeMin);
    // obj.insert("baseRangeMax", settings.baseRangeMax);
    // obj.insert("hideOutOfRange", settings.hideOutOfRange);
    return obj;
}

std::shared_ptr<QSpace::Visualize::Layers::LayerSettings>
ProjectSerializer::deserializeVisualSettings(const QJsonObject& json) {
    std::shared_ptr<QSpace::Visualize::Layers::LayerSettings> vs;
    // vs.mode = Visualize::rendermodeFromString(json["mode"].toString())
    //               .value_or(Visualize::RenderMode::Points);

    // if (json.contains("colorMapId")) {
    //     QUuid id = QUuid::fromString(json["colorMapId"].toString());
    //     // Ищем в менеджере (туда уже загрузились кастомные палитры из корня проекта)
    //     if (!Visualize::ColorMapManager::instance().contains(id)) {
    //         qCWarning(LogSession) << "don't exist colorMap:" << id.toString();
    //         id = Visualize::ColorMapPresets::getStandardPresets().first().id;
    //     }
    // } else if (json.contains("colorMap")) { // Легаси поддержка старых сохранений
    //     QString name  = json["colorMap"].toString();
    //     vs.colorMapId = Visualize::ColorMapPresets::getPresetByName(name).id;
    // } else {
    //     vs.colorMapId = Visualize::ColorMapPresets::getStandardPresets().first().id;
    // }

    // vs.isVisible                    = json["isVisible"].toBool(true);
    // vs.PointSize                    = json["pointSize"].toDouble(0.005);
    // vs.opacity                      = json["opacity"].toDouble(1.0);
    // vs.colorByField                 = json["colorByField"].toString();
    // vs.useLogScale                  = json["useLogScale"].toBool(false);
    // vs.showScalarBar                = json["showScalarBar"].toBool(true);
    // vs.autoRange                    = json["autoRange"].toBool(true);
    // vs.rangeMin                     = json["rangeMin"].toDouble(0.0);
    // vs.rangeMax                     = json["rangeMax"].toDouble(100.0);
    // vs.isEmmisive                   = json["isEmisive"].toBool(false);
    // vs.interpolationRangeType       = json["interpolationRangeType"].toString();
    // vs.ShaderType                   = json["shaderType"].toString();
    // vs.interpolationOpacityFunction = json["interpolationOpacityFunction"].toString();
    // vs.gaussianSharpness            = json["gaussianSharpness"].toDouble(4.5);
    // vs.sigmoidGammaOpacity          = json["sigmoidGammaOpacity"].toDouble(6.0);
    // vs.sigmoidShiftOpacity          = json["sigmoidShiftOpacity"].toDouble(0.2);
    // vs.sigmoidGammaColor            = json["sigmoidGammaColor"].toDouble(6.0);
    // vs.sigmoidShiftColor            = json["sigmoidShiftColor"].toDouble(0.2);
    // vs.alpha                        = json["alpha"].toDouble(3.0);
    // vs.baseRangeMin                 = json["baseRangeMin"].toDouble(0.0);
    // vs.baseRangeMax                 = json["baseRangeMax"].toDouble(100.0);
    // vs.hideOutOfRange               = json["hideOutOfRange"].toBool(true);
    // vs.exposureClamp                = json["exposureClamp"].toDouble(1.0);
    return vs;
}

// =========================================================================
// СХЕМЫ ЧТЕНИЯ
// =========================================================================

QJsonObject ProjectSerializer::serializeReadScheme(const IO::ReadScheme& scheme) {
    // return std::visit(
    //     [](auto&& arg) -> QJsonObject {
    //         using T = std::decay_t<decltype(arg)>;
    //         QJsonObject obj;
    //         if constexpr (std::is_same_v<T, IO::ColumnScheme>) {
    //             obj = serializeColumnScheme(arg);
    //             obj.insert("schemeType", "ColumnScheme");
    //         } else if constexpr (std::is_same_v<T, IO::HDF5ReadScheme>) {
    //             obj.insert("schemeType", "HDF5ReadScheme");
    //             obj.insert("path", arg.internalDatasetPath);
    //             obj.insert("loadAll", arg.loadAll);
    //             obj.insert("compression", arg.compressionLevel);
    //         } else if constexpr (std::is_same_v<T, IO::DefaultScheme>) {
    //             obj.insert("schemeType", "DefaultScheme");
    //         } else {
    //             obj.insert("schemeType", "None");
    //         }
    //         return obj;
    //     },
    //     scheme);
}

IO::ReadScheme ProjectSerializer::deserializeReadScheme(const QJsonObject& json) {
    // QString type = json["schemeType"].toString();
    // if (type == "ColumnScheme") {
    //     return deserializeColumnScheme(json);
    // }
    // if (type == "HDF5ReadScheme") {
    //     IO::HDF5ReadScheme s;
    //     s.internalDatasetPath = json["path"].toString();
    //     s.loadAll             = json["loadAll"].toBool();
    //     s.compressionLevel    = json["compression"].toInt();
    //     return s;
    // }
    // if (type == "DefaultScheme") {
    //     return IO::DefaultScheme{};
    // }
    // return std::monostate{};
}

QJsonObject ProjectSerializer::serializeColumnScheme(const IO::ColumnScheme& cs) {
    QJsonObject obj;
    obj.insert("offset", cs.headerOffsetBytes);
    obj.insert("interleaved", cs.isInterleaved);
    obj.insert("delimiter", cs.delimiter);

    QJsonArray cols;
    for (const auto& m : cs.columnsPolicy) {
        QJsonObject mObj;
        mObj.insert("name", m.name);
        mObj.insert("vtkType", m.vtkDataType);
        mObj.insert("role", m.vtkAttributeRole);
        mObj.insert("components", m.numberOfComponents);
        mObj.insert("isCoord", m.isCoordinate);
        cols.append(mObj);
    }
    obj.insert("columns", cols);
    return obj;
}

IO::ColumnScheme ProjectSerializer::deserializeColumnScheme(const QJsonObject& json) {
    // IO::ColumnScheme cs;
    // cs.headerOffsetBytes = json["offset"].toInt();
    // cs.isInterleaved     = json["interleaved"].toBool();
    // cs.delimiter         = json["delimiter"].toString();

    // QJsonArray cols = json["columns"].toArray();
    // for (auto v : cols) {
    //     QJsonObject               mObj = v.toObject();
    //     IO::ColumnScheme::Mapping m;
    //     m.name               = mObj["name"].toString();
    //     m.vtkDataType        = mObj["vtkType"].toInt();
    //     m.vtkAttributeRole   = mObj["role"].toInt();
    //     m.numberOfComponents = mObj["components"].toInt();
    //     m.isCoordinate       = mObj["isCoord"].toBool();
    //     cs.columnsPolicy.append(m);
    // }
    // return cs;
}

QJsonObject ProjectSerializer::serializeColorMap(const QSpace::Visualize::ColorMap& map) {
    QJsonObject obj;
    obj.insert("id", map.id.toString());
    obj.insert("name", map.name);
    obj.insert("filePath", map.filePath);
    obj.insert("isPreset", map.isPreset);

    QJsonArray ptsArr;
    for (const auto& pt : map.points) {
        QJsonObject p;
        p.insert("x", pt.x);
        p.insert("r", pt.r);
        p.insert("g", pt.g);
        p.insert("b", pt.b);
        ptsArr.append(p);
    }
    obj.insert("points", ptsArr);
    return obj;
}

QSpace::Visualize::ColorMap ProjectSerializer::deserializeColorMap(const QJsonObject& json) {
    QSpace::Visualize::ColorMap map;
    map.id       = QUuid::fromString(json["id"].toString());
    map.name     = json["name"].toString();
    map.filePath = json["filePath"].toString();
    map.isPreset = json["isPreset"].toBool();

    QJsonArray ptsArr = json["points"].toArray();
    for (auto v : ptsArr) {
        QJsonObject p = v.toObject();
        map.points.append(
            {p["x"].toDouble(), p["r"].toDouble(), p["g"].toDouble(), p["b"].toDouble()});
    }
    return map;
}

} // namespace QSpace::Session