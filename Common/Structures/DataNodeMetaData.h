#pragma once

namespace QSpace::Core {
struct DataNodeMetaData { // перенести вычисление метаданных в отдельный модуль physics
    double                               timestamp;
    double                               bounds[6];
    double                               center[3]       = {0, 0, 0};
    double                               centerOfMass[3] = {0, 0, 0};
    qint64                               pointCount;
    qint64                               cellCount;
    QMap<QString, QPair<double, double>> scalarRanges;

    bool operator==(const DataNodeMetaData& other) const {
        constexpr double eps = 1e-9;

        // Вспомогательное лямбда-выражение для сравнения double с эпсилон
        auto doubleEqual = [](double a, double b, double epsilon = 1e-9) {
            return std::abs(a - b) < epsilon;
        };

        // Сравнение timestamp
        if (!doubleEqual(timestamp, other.timestamp, eps)) {
            return false;
        }

        // Сравнение целочисленных полей
        if (pointCount != other.pointCount || cellCount != other.cellCount) {
            return false;
        }

        // Сравнение C-массива bounds (6 элементов)
        for (int i = 0; i < 6; ++i) {
            if (!doubleEqual(bounds[i], other.bounds[i], eps)) {
                return false;
            }
        }

        // Сравнение C-массивов center и centerOfMass (по 3 элемента)
        for (int i = 0; i < 3; ++i) {
            if (!doubleEqual(center[i], other.center[i], eps) ||
                !doubleEqual(centerOfMass[i], other.centerOfMass[i], eps)) {
                return false;
            }
        }

        // Сравнение QMap с диапазонами скаляров
        if (scalarRanges.size() != other.scalarRanges.size()) {
            return false;
        }

        for (auto it = scalarRanges.cbegin(); it != scalarRanges.cend(); ++it) {
            auto otherIt = other.scalarRanges.find(it.key());
            if (otherIt == other.scalarRanges.cend()) {
                return false; // Ключ не найден в other
            }

            // Сравниваем пары double (min, max)
            if (!doubleEqual(it.value().first, otherIt.value().first, eps) ||
                !doubleEqual(it.value().second, otherIt.value().second, eps)) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const DataNodeMetaData& other) const {
        return !(*this == other);
    }
};
} // namespace QSpace::Core