#pragma once
#include "Common/Enums/IOEnums.h"
#include "Common/Interfaces/IReader.h"
#include "Common/Structures/FileSchemeStructures.h"
#include <QFile>
#include <QtEndian>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtypes.h>
#include <vtkAbstractArray.h>
#include <vtkPointSet.h>
#include <vtkPolyData.h>
#include "../ReadResult.h"
#include <cstddef>
#include <functional>

namespace QSpace::IO {
class BINReader : public IReader {
  public:
    ~BINReader() override;

    void setPolicy(FilePolicy policy) override {
        m_policy = policy;
    }

    // Тип функции-парсера:
    // Вход: указатель на текущий байт файла (ptr), индекс текущей частицы (i)
    // Выход: указатель на следующий байт после прочитанного
    ReadResult read(const QString& path, const ReadScheme& scheme) const override;

  private:
    using ParserFunc    = std::function<const uchar*(const uchar*, int)>;
    FilePolicy m_policy = FilePolicy::Auto;

    struct ReadContext {
        vtkSmartPointer<vtkPolyData>             polyData;
        vtkSmartPointer<vtkPoints>               points;
        QList<vtkSmartPointer<vtkAbstractArray>> attributArrays;
        const ColumnScheme&                      config;
        const int                                N;
        const qint64                             dataStartPos;
        size_t                                   particleSize;
    };

    // ---подготовка структур данных из vtk---
    // создает массивы vtk и настраивает их на заданные данные
    bool prepareVTK(ReadContext& context) const;
    // назначает роля для атрибутов в vtk
    void assignVTKRoles(ReadContext& context) const;

    // ---политики чтения---
    // читает файлы через поток QStream
    bool readStream(QFile& file, ReadContext& context) const;
    // читает файлы через Mmap в виртуальную память
    bool readMmap(QFile& file, ReadContext& context, qint64 expectedDataSize) const;

    // ---стратегии чтения---
    bool
    readInterleavedMmap(const uchar* startPtr, ReadContext& context, double* rawPointsPtr) const;
    bool
    readNonInterleavedMmap(const uchar* startPtr, ReadContext& context, double* rawPointsPtr) const;
    bool readInterleavedStream(QFile& file, ReadContext& context, double* rawPointsPtr) const;
    bool readNonInterleavedStream(QFile& file, ReadContext& context, double* rawPointsPtr) const;

    //---вспомогательные функции Mmap
    QList<ParserFunc> generateParsers(const ReadContext& context, double* rawPointsPtr) const;
    bool              readColumnMmap(const uchar*         columnPtr,
                                     int                  N,
                                     const ColumnMapping& col,
                                     double*              rawPointsPtr,
                                     vtkAbstractArray*    attributArrayPtr) const;
    bool              readColumnStream(QFile&               file,
                                       int                  N,
                                       const ColumnMapping& col,
                                       double*              rawPointsPtr,
                                       vtkAbstractArray*    attributArrayPtr) const;

    template <class T>
    static void processCoordChunk(const uchar* srcBuffer,
                                  double*      rawPointsPtr,
                                  size_t       indParticle,
                                  size_t       count,
                                  int          indComp);
    template <class TSrc, class TDst>
    static void processAttribChunk(const uchar* srcBuffer,
                                   TDst*        attribArray,
                                   size_t       indParticle,
                                   size_t       count,
                                   int          nComp,
                                   int          indComp);
    void        createCells(ReadContext& context) const;
    bool        validate(const QFile& file, const ReadScheme& scheme) const;
};

// Реализация шаблонных методов
template <class T>
void BINReader::processCoordChunk(const uchar* srcBuffer,
                                  double*      rawPointsPtr,
                                  size_t       indParticle,
                                  size_t       count,
                                  int          indComp) {
    if (indComp > 3)
        return;
    const T* src = reinterpret_cast<const T*>(srcBuffer);
    double*  dst = rawPointsPtr + (indParticle * 3) + indComp;
    for (size_t i = 0; i < count; ++i) {
        *dst = static_cast<double>(qFromLittleEndian(src[i]));
        dst += 3;
    }
}

template <class TSrc, class TDst>
void BINReader::processAttribChunk(const uchar* srcBuffer,
                                   TDst*        attribArray,
                                   size_t       indParticle,
                                   size_t       count,
                                   int          nComp,
                                   int          indComp) {
    const TSrc* src = reinterpret_cast<const TSrc*>(srcBuffer);
    TDst*       dst = attribArray + (indParticle * nComp) + indComp;
    for (size_t i = 0; i < count; ++i) {
        *dst = static_cast<TDst>(qFromLittleEndian(src[i]));
        dst += nComp;
    }
}
} // namespace QSpace::IO