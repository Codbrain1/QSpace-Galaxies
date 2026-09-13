#pragma once
#include "Common/Enums/IOEnums.h"
#include "Common/Enums/VisualizeBaseEnums.h"
#include "Common/Structures/FileSchemeStructures.h"

#include <qobject.h>

namespace QSpace::IO {
class ReadSchemePresets {
  public:
    static QList<QPair<PresetKey, ReadScheme>> getStandardPresets();
    static QString                             presetDisplayName(const PresetKey& key);

  private:
    static ReadScheme createScheme_v2(Visualize::EntityType type, FileFormat format);
    static ReadScheme createScheme_v2_3(Visualize::EntityType type, FileFormat format);
};

} // namespace QSpace::IO