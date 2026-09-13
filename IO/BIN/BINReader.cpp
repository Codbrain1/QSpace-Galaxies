#include "BINReader.h"
#include "Common/Logger/Logger.h"
#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <qassert.h>
#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qendian.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qsharedpointer.h>
#include <qstringview.h>
#include <qtypes.h>
#include <vtkAbstractArray.h>
#include <vtkCell.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkType.h>
#include <cstddef>
#include <variant>

namespace QSpace::IO {
BINReader::~BINReader() = default;

ReadResult BINReader::read(const QString& path, const ReadScheme& scheme) const {
    // >---------------   1. ===== Валидация данных =====   ---------------<
    const auto& config = std::get<ColumnScheme>(scheme);
    if (!path.endsWith(".bin")) {
        qCCritical(LogIO) << "BinReader requires .bin file format: " + path;
        return {nullptr,
                path,
                "BinReader requires .bin file format: " + path,
                0,
                0,
                FileFormat::BIN,
                scheme,
                ReadStatus::InvalidFormat};
    }
    // открываем файл
    QFile file(path);
    auto  isValid = validate(file, scheme); // проверяем что переданная структура файла валидна
    if (!isValid) {
        return {nullptr,
                path,
                "Unsupported file structure: " + path,
                0,
                0,
                FileFormat::BIN,
                scheme,
                ReadStatus::InvalidFileStructure};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(LogIO) << "file " << file.fileName() << "is not Open!";
        return {nullptr,
                path,
                "Could not open file: " + path,
                0,
                0,
                FileFormat::BIN,
                scheme,
                ReadStatus::FileNotFound};
    }
    // обработка заголовка
    if (config.headerOffsetBytes > 0) {
        if (!file.seek(config.headerOffsetBytes)) { // пропуск незначимых данных
            qCCritical(LogIO) << "BinReader requires file header: NumParticles, time "
                                 "(int,double)[realHeaderOffset]"
                              << config.headerOffsetBytes;
            return {nullptr,
                    path,
                    "Could not read file header: " + path,
                    0,
                    0,
                    FileFormat::BIN,
                    scheme,
                    ReadStatus::InvalidFileStructure};
        }
    }

    // >---------------   2. ===== Чтение заголовка =====   ---------------<
    QDataStream headerStream(&file);
    headerStream.setByteOrder(QDataStream::LittleEndian);
    headerStream.setFloatingPointPrecision(
        QDataStream::DoublePrecision); // TODO: может выдать некоорректные данные если в заголовке
                                       // вместо double, записано float

    int    N         = 0;
    double timestamp = 0;
    headerStream >> N;
    headerStream >> timestamp;
    if (N <= 0) {
        qCritical(LogIO) << "Incorrect number of particles: " << N;
        return {nullptr,
                path,
                "Incorrect number of particles in file " + path,
                0,
                0,
                FileFormat::BIN,
                scheme,
                ReadStatus::InvalidFileStructure};
    }
    qCDebug(LogIO) << "Reading N = " << N << "; timestamp = " << timestamp
                   << " in File: " << file.fileName();

    // >---------------   3. ===== Подготовка контекста файла =====   ---------------<
    // Рассчет занимаемой памяти для одной частицы
    size_t particleSize = 0;
    for (const auto& col : config.columnsPolicy) {
        size_t colSize = 0;
        switch (col.vtkDataType) {
            case VTK_DOUBLE:
                colSize = sizeof(double);
                break;
            case VTK_FLOAT: {
                colSize = sizeof(float);
                break;
            }
            case VTK_INT: {
                colSize = sizeof(int);
                break;
            }
            default: {
                Q_ASSERT(false);
                return {nullptr,
                        path,
                        "should never be called",
                        0,
                        0,
                        FileFormat::BIN,
                        scheme,
                        ReadStatus::UnknownError};
            }
        }
        // Используем numberOfComponents для учета всех компонент (не только координат)
        colSize *= col.numberOfComponents;
        particleSize += colSize;
    }
    qCDebug(LogIO) << "ParticleSize calculation: total=" << particleSize << "bytes for"
                   << config.columnsPolicy.size() << "columns";
    qint64 dataStart = config.headerOffsetBytes + sizeof(int) + sizeof(double);
    // подготавливем vtk примитивы
    ReadContext context{vtkSmartPointer<vtkPolyData>::New(),
                        vtkSmartPointer<vtkPoints>::New(),
                        {}, // будет проинициализирован в prepareVTK
                        config,
                        N,
                        dataStart,
                        particleSize};

    // >---------------   4. ===== Валидация данных =====   ---------------<
    auto isPrerape = prepareVTK(context);
    if (!isPrerape)
        return {nullptr,
                path,
                "vtk couldn't create data structures",
                0,
                0,
                FileFormat::BIN,
                scheme,
                ReadStatus::UnknownError};

    // >---------------   5. ===== Выбор политики чтения =====   ---------------<
    bool UseMap = false;
    if (m_policy == FilePolicy::ForceMapped ||
        (m_policy == FilePolicy::Auto && file.size() >= 100 * 1024 * 1024))
        UseMap = true;
    bool success = false;

    // проверка размера файла
    qint64 expectedDataSize = static_cast<qint64>(context.N) * particleSize;
    if (file.size() - context.dataStartPos < expectedDataSize) {
        qCCritical(LogIO) << "File size less expected for " << context.N << " particles";
        return {nullptr,
                path,
                "Error file size:" + path,
                0,
                0,
                FileFormat::BIN,
                scheme,
                ReadStatus::UnknownError};
    }

    // >---------------   6. ===== Чтение файлов =====   ---------------<
    if (UseMap) {
        // проецирование в виртуальную память
        success = readMmap(file, context, expectedDataSize);
        if (!success) {
            qCWarning(LogIO) << "Mmaping file into virtual memory failed/unsupported. Falling back "
                                "to standart read";
            file.seek(context.dataStartPos);
            success = readStream(file, context);
        }

    } else {
        // обычное потоковое чтение
        success = readStream(file, context);
    }
    if (!success) {
        qCCritical(LogIO) << "Failed read file: " << file.fileName();
        return {nullptr,
                path,
                "Failed read file: " + path,
                0,
                0,
                FileFormat::BIN,
                scheme,
                ReadStatus::UnknownError};
    }
    context.polyData->SetPoints(context.points);
    auto pd = context.polyData->GetPointData();
    for (auto arr : context.attributArrays) { // добавляем атрибуты
        pd->AddArray(arr);
    }
    // TODO:: исправить задание ролей по умолчанию
    //  задаем роли
    for (const auto& col : config.columnsPolicy) {
        if (col.vtkAttributeRole != -1) {
            vtkAbstractArray* arr = pd->GetAbstractArray(col.name.toStdString().c_str());
            if (arr) {
                if (pd->GetAttribute(col.vtkAttributeRole) == nullptr) {
                    pd->SetAttribute(arr, col.vtkAttributeRole);
                }
                // вывод для отладки
                //  for (int k = 0; k < context.polyData->GetPointData()->GetNumberOfArrays(); ++k)
                //  {
                //      qCDebug(LogIO) << context.polyData->GetPointData()->GetArrayName(k);
                //  }
                //  qCDebug(LogIO) << "end";
            }
        }
    }

    createCells(context);
    IO::ReadResult res;
    res.data       = context.polyData;
    res.path       = path;
    res.errMessage = "";
    res.timestamp  = timestamp;
    res.status     = ReadStatus::Succes;
    res.format     = FileFormat::BIN;
    res.scheme     = scheme;
    return res;
}

bool BINReader::validate(const QFile& file, const ReadScheme& scheme) const {
    if (!std::holds_alternative<ColumnScheme>(scheme)) {
        qCritical(LogIO) << "BinReader requires ComlumnScheme! file:" << file.fileName();
        return false;
    }
    const auto& config = std::get<ColumnScheme>(scheme);
    for (const auto& col : config.columnsPolicy) {
        if (col.vtkDataType != VTK_DOUBLE && col.vtkDataType != VTK_FLOAT &&
            col.vtkDataType != VTK_INT) {
            qCCritical(LogIO) << "Unsuported type in column: " << col.name;
            return false;
        }
        if (col.name.isEmpty()) {
            qCCritical(LogIO) << "Column name is empty!";
            return false;
        }
        if (col.numberOfComponents < 1) {
            qCCritical(LogIO) << "Number of Components can't less one!";
            return false;
        }
    }
    return true;
}

bool BINReader::prepareVTK(BINReader::ReadContext& context) const {
    context.points->SetDataTypeToDouble();        // задаем double для координат
    context.points->SetNumberOfPoints(context.N); // задаем общее число точек

    // Список атрибутов: Создаем массивы на основе types из columnsPolicy
    // Также запоминаем роли для установки активных атрибутов (Scalars, Vectors и т.д.)
    for (const auto& col : context.config.columnsPolicy) {
        if (col.isCoordinate)
            continue;
        vtkSmartPointer<vtkAbstractArray> array;
        switch (col.vtkDataType) {
            case VTK_DOUBLE: {
                auto doubleArray = vtkSmartPointer<vtkDoubleArray>::New();
                doubleArray->SetNumberOfComponents(col.numberOfComponents);
                doubleArray->SetNumberOfTuples(context.N);
                array = doubleArray;
                break;
            }
            case VTK_FLOAT: {
                auto doubleArray = vtkSmartPointer<vtkFloatArray>::New();
                doubleArray->SetNumberOfComponents(col.numberOfComponents);
                doubleArray->SetNumberOfTuples(context.N);
                array = doubleArray;
                break;
            }
            case VTK_INT: {
                auto intArray = vtkSmartPointer<vtkIntArray>::New();
                intArray->SetNumberOfComponents(col.numberOfComponents);
                intArray->SetNumberOfTuples(context.N);
                array = intArray;
                break;
            }
            default: {
                Q_ASSERT(false);
                return false;
            }
        }
        if (array) {
            array->SetName(col.name.toStdString().c_str());
            context.attributArrays.append(array);
        }
    }
    return true;
}

QList<BINReader::ParserFunc> BINReader::generateParsers(const ReadContext& context,
                                                        double*            rawPointsPtr) const {
    QList<ParserFunc> parsers;
    int               attrrArrayIndex = 0;
    for (const auto& col : context.config.columnsPolicy) {
        if (col.isCoordinate) {
            switch (col.vtkDataType) { // ПАРСЕР координат
                case VTK_DOUBLE: {
                    parsers.append([rawPointsPtr](const uchar* ptr, int i) {
                        const double* src = reinterpret_cast<const double*>(ptr);
                        double*       dst = rawPointsPtr + (3 * i);
                        for (int c = 0; c < 3; ++c) {
                            dst[c] = qFromLittleEndian(src[c]);
                        }
                        return ptr + sizeof(double) * 3;
                    });
                    break;
                }
                case VTK_FLOAT: {
                    parsers.append([rawPointsPtr](const uchar* ptr, int i) {
                        const float* src = reinterpret_cast<const float*>(ptr);
                        double*      dst = rawPointsPtr + (3 * i);
                        for (int c = 0; c < 3; ++c) {
                            dst[c] = static_cast<double>(qFromLittleEndian(src[c]));
                        }
                        return ptr + sizeof(float) * 3;
                    });
                    break;
                }
                default:
                    Q_ASSERT(false);
                    return QList<BINReader::ParserFunc>();
            }
        } else {
            vtkAbstractArray* arr = context.attributArrays[attrrArrayIndex++];
            switch (col.vtkDataType) {
                case VTK_DOUBLE: {
                    vtkDoubleArray* doubleArr = vtkDoubleArray::SafeDownCast(arr);
                    parsers.append(
                        [nComp = col.numberOfComponents, doubleArr](const uchar* ptr, int i) {
                            const double* src = reinterpret_cast<const double*>(ptr);
                            double        buffer[16]; // Запас для компонент (обычно их < 9)
                            for (int k = 0; k < nComp; ++k) {
                                buffer[k] = qFromLittleEndian(src[k]);
                            }
                            doubleArr->SetTypedTuple(i, buffer);
                            return ptr + sizeof(double) * nComp;
                        });
                    break;
                }
                case VTK_FLOAT: {
                    vtkFloatArray* floatArr = vtkFloatArray::SafeDownCast(arr);
                    parsers.append(
                        [nComp = col.numberOfComponents, floatArr](const uchar* ptr, int i) {
                            const float* src = reinterpret_cast<const float*>(ptr);
                            float        buffer[16]; // Запас для компонент (обычно их < 9)
                            for (int k = 0; k < nComp; ++k) {
                                buffer[k] = qFromLittleEndian(src[k]);
                            }
                            floatArr->SetTypedTuple(i, buffer);
                            return ptr + sizeof(float) * nComp;
                        });
                    break;
                }
                case VTK_INT: {
                    vtkIntArray* intArr = vtkIntArray::SafeDownCast(arr);
                    parsers.append(
                        [nComp = col.numberOfComponents, intArr](const uchar* ptr, int i) {
                            const int* src = reinterpret_cast<const int*>(ptr);
                            int        buffer[16]; // Запас для компонент (обычно их < 9)
                            for (int k = 0; k < nComp; ++k) {
                                buffer[k] = qFromLittleEndian(src[k]);
                            }
                            intArr->SetTypedTuple(i, buffer);
                            return ptr + sizeof(int) * nComp;
                        });
                    break;
                }
                default:
                    Q_ASSERT(false);
                    return QList<BINReader::ParserFunc>();
            }
        }
    }
    return parsers;
}

bool BINReader::readColumnMmap(const uchar*         columnPtr,
                               int                  N,
                               const ColumnMapping& col,
                               double*              rawPointsPtr,
                               vtkAbstractArray*    attributArrayPtr) const {
    int nComp = col.numberOfComponents;
    if (col.isCoordinate) { // Координаты
        switch (col.vtkDataType) {
            case VTK_DOUBLE: {
                const double* src = reinterpret_cast<const double*>(columnPtr);
                for (int c = 0; c < 3; ++c) {
                    const double* componentSrc = src + (static_cast<size_t>(c) * N);
                    double*       dst          = rawPointsPtr + c;
                    for (int i = 0; i < N; ++i) {
                        *dst = qFromLittleEndian(componentSrc[i]);
                        dst += 3;
                    }
                }
                break;
            }
            case VTK_FLOAT: {
                const float* src = reinterpret_cast<const float*>(columnPtr);
                for (int c = 0; c < 3; ++c) {
                    const float* componentSrc = src + (static_cast<size_t>(c) * N);
                    double*      dst          = rawPointsPtr + c;
                    for (int i = 0; i < N; ++i) {
                        *dst = static_cast<double>(qFromLittleEndian(componentSrc[i]));
                        dst += 3;
                    }
                }
                break;
            }
            default: {
                Q_ASSERT(false);
                return false;
            }
        }

    } else if (attributArrayPtr) { // Атрибутивная колонка (скорость, плотность...)
        switch (col.vtkDataType) {
            case VTK_DOUBLE: {
                const double*   src       = reinterpret_cast<const double*>(columnPtr);
                vtkDoubleArray* doubleArr = vtkDoubleArray::SafeDownCast(attributArrayPtr);
                double*         dst       = doubleArr->GetPointer(0);
                for (int c = 0; c < nComp; ++c) {
                    const double* componentSrc = src + (c * N);
                    for (int i = 0; i < N; ++i) {
                        dst[i * nComp + c] =
                            qFromLittleEndian(componentSrc[i]); // TODO: оптимизировать чтение за
                                                                // счет инкрементального dst
                    }
                }
                break;
            }
            case VTK_FLOAT: {
                const float*   src      = reinterpret_cast<const float*>(columnPtr);
                vtkFloatArray* floatArr = vtkFloatArray::SafeDownCast(attributArrayPtr);
                float*         fst      = floatArr->GetPointer(0);
                for (int c = 0; c < nComp; ++c) {
                    const float* componentSrc = src + (c * N);
                    for (int i = 0; i < N; ++i) {
                        fst[i * nComp + c] = qFromLittleEndian(componentSrc[i]);
                    }
                }
                break;
            }
            case VTK_INT: {
                const int*   src    = reinterpret_cast<const int*>(columnPtr);
                vtkIntArray* intArr = vtkIntArray::SafeDownCast(attributArrayPtr);
                int*         ist    = intArr->GetPointer(0);
                for (int c = 0; c < nComp; ++c) {
                    const int* componentSrc = src + (c * N);
                    for (int i = 0; i < N; ++i) {
                        ist[i * nComp + c] = qFromLittleEndian(componentSrc[i]);
                    }
                }
                break;
            }
            default: {
                Q_ASSERT(false);
                return false;
            }
        }
    }
    return true;
}

void BINReader::createCells(BINReader::ReadContext& context) const {
    auto cells = vtkSmartPointer<vtkCellArray>::New();
#ifdef VTK_VERSION_NUMBER
#if VTK_VERSION_NUMBER >= 9000000000ULL
    cells->AllocateEstimate(context.N, 1);
    for (vtkIdType i = 0; i < context.N; ++i)
        cells->InsertNextCell(1, &i);
#else
    cells->Allocate(context.N);
    for (vtkIdType i = 0; i < context.N; ++i)
        cells->InsertNextCell(1, &i);
#endif
#else
    cells->Allocate(context.N);
    for (vtkIdType i = 0; i < context.N; ++i)
        cells->InsertNextCell(1, &i);
#endif
    context.polyData->SetVerts(cells);
}

bool BINReader::readMmap(QFile& file, ReadContext& context, qint64 expectedDataSize) const {
    // проецируем файл в виртуальную память
    uchar* mappedData = file.map(context.dataStartPos, expectedDataSize);
    if (!mappedData) {
        qCCritical(LogIO) << "File " << file.fileName() << " is not mapped into virtual memory";
        return false;
    }
    vtkDoubleArray* coordArray   = vtkDoubleArray::SafeDownCast(context.points->GetData());
    double*         rawPointsPtr = coordArray->GetPointer(0);
    bool            res          = false;
    if (context.config.isInterleaved) {
        res = readInterleavedMmap(mappedData, context, rawPointsPtr);
    } else {
        res = readNonInterleavedMmap(mappedData, context, rawPointsPtr);
    }
    file.unmap(mappedData);
    return res;
}

bool BINReader::readInterleavedMmap(const uchar* startPtr,
                                    ReadContext& context,
                                    double*      rawPointsPtr) const {
    auto parsers = generateParsers(context, rawPointsPtr);
    if (parsers.isEmpty())
        return false;
    const uchar* currentPtr = startPtr;
    for (int i = 0; i < context.N; ++i) {
        for (const auto& parse : parsers) {
            currentPtr = parse(currentPtr, i);
        }
    }
    return true;
}

bool BINReader::readNonInterleavedMmap(const uchar* startPtr,
                                       ReadContext& context,
                                       double*      rawPointsPtr) const {
    int          attrIndex  = 0;
    const uchar* currnetPtr = startPtr;
    for (const auto& col : context.config.columnsPolicy) {
        size_t column_size;
        switch (col.vtkDataType) {
            case VTK_DOUBLE: {
                column_size = static_cast<size_t>(context.N) * sizeof(double);
                break;
            }
            case VTK_FLOAT: {
                column_size = static_cast<size_t>(context.N) * sizeof(float);
                break;
            }
            case VTK_INT: {
                column_size = static_cast<size_t>(context.N) * sizeof(int);
                break;
            }
            default: {
                Q_ASSERT(false);
                return false;
            }
        }
        vtkAbstractArray* arr = nullptr;
        if (!col.isCoordinate) {
            arr = context.attributArrays[attrIndex++];
        }
        bool res = readColumnMmap(currnetPtr, context.N, col, rawPointsPtr, arr);
        if (!res)
            return false;
        if (col.isCoordinate)
            column_size *= 3;
        currnetPtr += column_size;
    }
    return true;
}

bool BINReader::readStream(QFile& file, ReadContext& context) const {
    vtkDoubleArray* coordArray   = vtkDoubleArray::SafeDownCast(context.points->GetData());
    double*         rawPointsPtr = coordArray->GetPointer(0);
    bool            res          = false;
    if (context.config.isInterleaved) {
        res = readInterleavedStream(file, context, rawPointsPtr);
    } else {
        res = readNonInterleavedStream(file, context, rawPointsPtr);
    }
    return res;
}

bool BINReader::readColumnStream(QFile&               file,
                                 int                  N,
                                 const ColumnMapping& col,
                                 double*              rawPointsPtr,
                                 vtkAbstractArray*    attributArrayPtr) const {
    constexpr int CHUNK_SIZE = 65536; // 64Кб
    QByteArray    buffer(CHUNK_SIZE, 0);
    char*         bufferPtr = buffer.data();
    int           nComp     = col.numberOfComponents;
    size_t        typeSize  = 0;
    switch (col.vtkDataType) {
        case VTK_DOUBLE: {
            typeSize = sizeof(double);
            break;
        }
        case VTK_FLOAT: {
            typeSize = sizeof(float);
            break;
        }
        case VTK_INT: {
            typeSize = sizeof(int);
            break;
        }
        default:
            Q_ASSERT(false);
            return false;
    }
    const int chunkElements = CHUNK_SIZE / typeSize;
    for (int i = 0; i < nComp; ++i) {
        size_t particlesRead = 0;
        while (particlesRead < static_cast<size_t>(N)) {
            int toRead      = std::min(static_cast<size_t>(chunkElements),
                                       static_cast<size_t>(N) - particlesRead);
            int bytesToRead = toRead * typeSize;
            if (file.read(bufferPtr, bytesToRead) != bytesToRead) {
                qCCritical(LogIO) << "Unexpected end of file reading column:" << col.name;
                return false;
            }
            const uchar* uBufferPtr = reinterpret_cast<const uchar*>(bufferPtr);
            if (col.isCoordinate) {
                switch (col.vtkDataType) {
                    case VTK_DOUBLE: {
                        processCoordChunk<double>(uBufferPtr,
                                                  rawPointsPtr,
                                                  particlesRead,
                                                  toRead,
                                                  i);
                        break;
                    }
                    case VTK_FLOAT: {
                        processCoordChunk<float>(uBufferPtr,
                                                 rawPointsPtr,
                                                 particlesRead,
                                                 toRead,
                                                 i);
                        break;
                    }
                    default:
                        Q_ASSERT(false);
                        return false;
                }
            } else if (attributArrayPtr) {
                switch (col.vtkDataType) {
                    case VTK_DOUBLE: {
                        vtkDoubleArray* arr = vtkDoubleArray::SafeDownCast(attributArrayPtr);
                        processAttribChunk<double, double>(uBufferPtr,
                                                           arr->GetPointer(0),
                                                           particlesRead,
                                                           toRead,
                                                           nComp,
                                                           i);
                        break;
                    }
                    case VTK_FLOAT: {
                        vtkFloatArray* arr = vtkFloatArray::SafeDownCast(attributArrayPtr);
                        processAttribChunk<float, float>(uBufferPtr,
                                                         arr->GetPointer(0),
                                                         particlesRead,
                                                         toRead,
                                                         nComp,
                                                         i);
                        break;
                    }
                    case VTK_INT: {
                        vtkIntArray* arr = vtkIntArray::SafeDownCast(attributArrayPtr);
                        processAttribChunk<int, int>(uBufferPtr,
                                                     arr->GetPointer(0),
                                                     particlesRead,
                                                     toRead,
                                                     nComp,
                                                     i);
                        break;
                    }
                    default:
                        Q_ASSERT(false);
                        return false;
                }
            }
            particlesRead += toRead;
        }
    }
    return true;
}

bool BINReader::readInterleavedStream(QFile&       file,
                                      ReadContext& context,
                                      double*      rawPointsPtr) const {
    QList<BINReader::ParserFunc> parsers = generateParsers(context, rawPointsPtr);
    if (parsers.isEmpty())
        return false;
    size_t     stride = context.particleSize;
    QByteArray rowBuffer(stride, 0);
    uchar*     rowPtr = reinterpret_cast<uchar*>(rowBuffer.data());

    qCDebug(LogIO) << "readInterleavedStream: stride=" << stride << "bytes, N=" << context.N;

    for (int i = 0; i < context.N; ++i) {
        if (file.read(reinterpret_cast<char*>(rowPtr), stride) != static_cast<qint64>(stride)) {
            qCCritical(LogIO) << "Unexpected end file at particle!";
            return false;
        }
        const uchar* current_ptr = rowPtr;
        for (const auto& parser : parsers) {
            current_ptr = parser(current_ptr, i);
        }

        // Выводим первые 3 частицы для отладки
        if (i < 3) {
            const double* dptr = reinterpret_cast<const double*>(rowBuffer.data());
            qCDebug(LogIO) << "Particle" << i << ": pos=(" << qFromLittleEndian(dptr[0]) << ","
                           << qFromLittleEndian(dptr[1]) << "," << qFromLittleEndian(dptr[2]) << ")"
                           << " vel=(" << qFromLittleEndian(dptr[3]) << ","
                           << qFromLittleEndian(dptr[4]) << "," << qFromLittleEndian(dptr[5]) << ")"
                           << " mass=" << qFromLittleEndian(dptr[6]);
        }
    }
    return true;
}

bool BINReader::readNonInterleavedStream(QFile&       file,
                                         ReadContext& context,
                                         double*      rawPointsPtr) const {
    int attrrArrayIndex = 0;
    for (const auto& col : context.config.columnsPolicy) {
        vtkAbstractArray* arr = nullptr;
        if (!col.isCoordinate) {
            arr = context.attributArrays[attrrArrayIndex++];
        }
        auto res = readColumnStream(file, context.N, col, rawPointsPtr, arr);
        if (!res)
            return false;
    }
    return true;
}

} // namespace QSpace::IO