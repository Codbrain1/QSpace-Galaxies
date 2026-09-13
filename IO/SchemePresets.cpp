#include "SchemePresets.h"
#include "Common/Structures/FileSchemeStructures.h"
#include <QList>
#include <qmetaobject.h>
#include <qvariant.h>
#include <vtkDataSet.h>
#include <vtkDataSetAttributes.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkSmartPointer.h>
#include <vtkType.h>
#include "Enums/VisualizeBaseEnums.h"

namespace QSpace::IO {
QList<QPair<PresetKey, ReadScheme>> ReadSchemePresets::getStandardPresets() {
    QList<QPair<PresetKey, ReadScheme>> result;

    auto add = [&](Visualize::EntityType   type,
                   FileFormat              format,
                   ModelingProgrammVersion mpv,
                   const ReadScheme&       scheme) {
        result.append({PresetKey{type, format, mpv}, scheme});
    };

    for (auto type : {Visualize::EntityType::DarkMatter,
                      Visualize::EntityType::Stars,
                      Visualize::EntityType::Gas}) {
        add(type,
            FileFormat::BIN,
            ModelingProgrammVersion::V2,
            createScheme_v2(type, FileFormat::BIN));
        add(type,
            FileFormat::BIN,
            ModelingProgrammVersion::V2_3,
            createScheme_v2_3(type, FileFormat::BIN));
    }
    return result;
}

QString ReadSchemePresets::presetDisplayName(const PresetKey& key) {
    QString name   = "scheme";
    QString type   = QVariant::fromValue(key.type).toString();
    QString format = QVariant::fromValue(key.format).toString();
    QString mpv    = QVariant::fromValue(key.mpv).toString();
    return QString("%1_%2_%3_%4").arg(name).arg(type).arg(format).arg(mpv);
}

ReadScheme ReadSchemePresets::createScheme_v2_3(Visualize::EntityType type, FileFormat format) {
    if (format == FileFormat::BIN || format == FileFormat::TXT) {
        ColumnScheme scheme;
        if (type == QSpace::Visualize::EntityType::DarkMatter ||
            type == QSpace::Visualize::EntityType::Stars) {
            scheme.columnsPolicy.append({VTK_DOUBLE, -1, 3, true, "Position"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::VECTORS, 3, false, "Velocity"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Mass"});
        } else if (type == QSpace::Visualize::EntityType::Gas) {
            scheme.columnsPolicy.append({VTK_DOUBLE, -1, 3, true, "Position"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Density"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::VECTORS, 3, false, "Velocity"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Energy"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Mass"});
            scheme.columnsPolicy.append(
                {VTK_INT, vtkDataSetAttributes::SCALARS, 1, false, "ind_SPH"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "t_MCYS"});
        }
        scheme.isInterleaved = true;
        return scheme;
    }
    // MINOR::добавить обработку других форматов hdf5 и тд
    return DefaultScheme{};
}

ReadScheme ReadSchemePresets::createScheme_v2(Visualize::EntityType type, FileFormat format) {
    if (format == FileFormat::BIN || format == FileFormat::TXT) {
        ColumnScheme scheme;
        if (type == QSpace::Visualize::EntityType::DarkMatter ||
            type == QSpace::Visualize::EntityType::Stars) {
            scheme.columnsPolicy.append({VTK_DOUBLE, -1, 3, true, "Position"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::VECTORS, 3, false, "Velocity"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Mass"});
        } else if (type == QSpace::Visualize::EntityType::Gas) {
            scheme.columnsPolicy.append({VTK_DOUBLE, -1, 3, true, "Position"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Density"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::VECTORS, 3, false, "Velocity"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Energy"});
            scheme.columnsPolicy.append(
                {VTK_DOUBLE, vtkDataSetAttributes::SCALARS, 1, false, "Mass"});
            scheme.columnsPolicy.append(
                {VTK_INT, vtkDataSetAttributes::SCALARS, 1, false, "ind_SPH"});
        }
        // MINOR:: //добавить обработку MIXED
        scheme.isInterleaved = true;
        return scheme;
    }
    // MINOR::добавить обработку других форматов hdf5 и тд
    return DefaultScheme{};
}
} // namespace QSpace::IO