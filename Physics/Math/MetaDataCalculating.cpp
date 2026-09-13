#include "MetaDataCalculating.h"
#include <vtkDataArray.h>
#include <vtkPointData.h>

namespace QSpace::Physics::Math {
QSpace::Core::DataNodeMetaData calculateMetaData(vtkSmartPointer<vtkDataSet> dataSet,
                                                 double                      timestamp) {
    QSpace::Core::DataNodeMetaData stats;

    if (dataSet) {
        dataSet->GetBounds(stats.bounds);
        stats.pointCount = dataSet->GetNumberOfPoints();
        stats.cellCount  = dataSet->GetNumberOfCells();

        stats.center[0] = (stats.bounds[0] + stats.bounds[1]) / 2.0;
        stats.center[1] = (stats.bounds[2] + stats.bounds[3]) / 2.0;
        stats.center[2] = (stats.bounds[4] + stats.bounds[5]) / 2.0;

        stats.timestamp  = timestamp;
        vtkPointData* pd = dataSet->GetPointData();
        if (pd) {
            for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
                auto array = pd->GetArray(i);
                if (array) {
                    double range[2];
                    array->GetRange(range);
                    stats.scalarRanges.insert(array->GetName(), {range[0], range[1]});
                }
            }
        }
    }
    return stats;
}

double calculateTimestamp(double timestamp) {
    double Km  = 3.72;
    double Kr  = 0.9;
    double l_v = 65.76 * sqrt(Km / Kr); // km/s
    double l_r = 10 * Kr;               // kpc
    double l_t = l_r / l_v * 1000.0 * 0.9784;
    return timestamp * l_t;
}
} // namespace QSpace::Physics::Math