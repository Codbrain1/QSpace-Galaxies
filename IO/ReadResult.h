#pragma once
#include "Common/Enums/IOEnums.h"
#include "Common/Structures/FileSchemeStructures.h"
#include <vtkDataSet.h>
#include <vtkMultiBlockDataSet.h>

namespace QSpace::IO {
struct ReadResult { // результат чтения одного файла
    vtkSmartPointer<vtkDataSet> data = nullptr;
    QString                     path;
    QString                     errMessage;
    double                      timestamp;
    double                      pointCount;
    IO::FileFormat              format;
    IO::ReadScheme              scheme;
    ReadStatus                  status = ReadStatus::UnknownError;

    bool isSuccess() const {
        return status == ReadStatus::Succes && data != nullptr;
    }
};

struct BatchResult { // результат чтения пакета файлов
    vtkSmartPointer<vtkMultiBlockDataSet> data         = nullptr;
    int                                   succes_count = 0;
    int                                   fail_count   = 0;
    QStringList                           errMessages;

    bool hasErrors() {
        return fail_count > 0;
    }
};
} // namespace QSpace::IO