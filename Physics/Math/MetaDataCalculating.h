#pragma once
#include "Common/Structures/ObjectRegistryStructures.h"

namespace QSpace::Physics::Math {
QSpace::Core::DataNodeMetaData calculateMetaData(vtkSmartPointer<vtkDataSet> dataSet,
                                                 double                      timestamp);
double                         calculateTimestamp(double timestamp);
} // namespace QSpace::Physics::Math