#include "Common/Enums/IOEnums.h"
#include "Common/Structures/FileSchemeStructures.h"
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtest.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkType.h>
#include "IO/BIN/BINReader.h"
#include <functional>



using namespace QSpace::IO;

class TestBINReaderRead : public QObject {
    Q_OBJECT
  private slots:

    void TestRead_data() {
        QTest::addColumn<int>("N");
        QTest::addColumn<int>("mode");    // 0 - Interleaved, 1 - Columnar
        QTest::addColumn<int>("vtkType"); // VTK_DOUBLE, VTK_FLOAT, etc.
        QTest::addColumn<int>("coordType");
        QTest::addColumn<FilePolicy>("Policy");

        // add variable combination
        QTest::newRow("Double-Double-Interleaved-Stream")
            << 100 << 0 << VTK_DOUBLE << VTK_DOUBLE << FilePolicy::ForceStandart;
        QTest::newRow("Double-Double-Columnar-Stream")
            << 100 << 1 << VTK_DOUBLE << VTK_DOUBLE << FilePolicy::ForceStandart;
        QTest::newRow("Double-Double-Interleaved-MMap")
            << 100 << 0 << VTK_DOUBLE << VTK_DOUBLE << FilePolicy::ForceMapped;
        QTest::newRow("Double-Double-Columnar-Mmap")
            << 100 << 1 << VTK_DOUBLE << VTK_DOUBLE << FilePolicy::ForceMapped;

        QTest::newRow("Float-Double-Interleaved-Stream")
            << 100 << 0 << VTK_FLOAT << VTK_DOUBLE << FilePolicy::ForceStandart;
        QTest::newRow("Float-Double-Columnar-Stream")
            << 100 << 1 << VTK_FLOAT << VTK_DOUBLE << FilePolicy::ForceStandart;
        QTest::newRow("Float-Double-Interleaved-MMap")
            << 100 << 0 << VTK_FLOAT << VTK_DOUBLE << FilePolicy::ForceMapped;
        QTest::newRow("Float-Double-Columnar-Mmap")
            << 100 << 1 << VTK_FLOAT << VTK_DOUBLE << FilePolicy::ForceMapped;

        QTest::newRow("Int-Double-Interleaved-Stream")
            << 100 << 0 << VTK_INT << VTK_DOUBLE << FilePolicy::ForceStandart;
        QTest::newRow("Int-Double-Columnar-Stream")
            << 100 << 1 << VTK_INT << VTK_DOUBLE << FilePolicy::ForceStandart;
        QTest::newRow("Int-Double-Interleaved-MMap")
            << 100 << 0 << VTK_INT << VTK_DOUBLE << FilePolicy::ForceMapped;
        QTest::newRow("Int-Double-Columnar-Mmap")
            << 100 << 1 << VTK_INT << VTK_DOUBLE << FilePolicy::ForceMapped;

        QTest::newRow("Double-Float-Interleaved-Stream")
            << 100 << 0 << VTK_DOUBLE << VTK_FLOAT << FilePolicy::ForceStandart;
        QTest::newRow("Double-Float-Columnar-Stream")
            << 100 << 1 << VTK_DOUBLE << VTK_FLOAT << FilePolicy::ForceStandart;
        QTest::newRow("Double-Float-Interleaved-MMap")
            << 100 << 0 << VTK_DOUBLE << VTK_FLOAT << FilePolicy::ForceMapped;
        QTest::newRow("Double-Float-Columnar-Mmap")
            << 100 << 1 << VTK_DOUBLE << VTK_FLOAT << FilePolicy::ForceMapped;

        QTest::newRow("Float-Float-Interleaved-Stream")
            << 100 << 0 << VTK_FLOAT << VTK_FLOAT << FilePolicy::ForceStandart;
        QTest::newRow("Float-Float-Columnar-Stream")
            << 100 << 1 << VTK_FLOAT << VTK_FLOAT << FilePolicy::ForceStandart;
        QTest::newRow("Float-Float-Interleaved-MMap")
            << 100 << 0 << VTK_FLOAT << VTK_FLOAT << FilePolicy::ForceMapped;
        QTest::newRow("Float-Float-Columnar-Mmap")
            << 100 << 1 << VTK_FLOAT << VTK_FLOAT << FilePolicy::ForceMapped;

        QTest::newRow("Int-Float-Interleaved-Stream")
            << 100 << 0 << VTK_INT << VTK_FLOAT << FilePolicy::ForceStandart;
        QTest::newRow("Int-Float-Columnar-Stream")
            << 100 << 1 << VTK_INT << VTK_FLOAT << FilePolicy::ForceStandart;
        QTest::newRow("Int-Float-Interleaved-MMap")
            << 100 << 0 << VTK_INT << VTK_FLOAT << FilePolicy::ForceMapped;
        QTest::newRow("Int-Float-Columnar-Mmap")
            << 100 << 1 << VTK_INT << VTK_FLOAT << FilePolicy::ForceMapped;
    }

    void TestRead() {
        QFETCH(int, N);
        QFETCH(int, mode);
        QFETCH(int, vtkType);
        QFETCH(int, coordType);
        QFETCH(FilePolicy, Policy);
        bool    isInterleaved = (mode == 0);
        QString path          = CreateTestFile(N, mode, vtkType, coordType);
        auto    config        = createScheme(isInterleaved, vtkType, coordType);

        BINReader reader;
        reader.setPolicy(Policy);
        auto dataset = reader.read(path, config);

        QVERIFY2(dataset.data != nullptr, "Reader returned nullptr");
        vtkPolyData* polyData = vtkPolyData::SafeDownCast(dataset.data);
        QVERIFY(polyData != nullptr);
        QCOMPARE(polyData->GetNumberOfPoints(), N);
        double p[3];

        polyData->GetPoint(N / 2, p);
        QCOMPARE(p[0], (double)(N / 2));
        QCOMPARE(p[1], (double)(N / 2 * 2));
        vtkDataArray* arr = polyData->GetPointData()->GetArray("TestAttribute");
        QVERIFY(arr != nullptr);
        double expectedAttr = (vtkType == VTK_INT) ? (double)(N / 2) : (double)(N / 2 + 0.5);
        auto   value        = qAbs(arr->GetTuple1(N / 2));
        QVERIFY(qAbs(value - expectedAttr) < 1e-6);
        QFile::remove(path);
    }

  private:
    template <typename T> void writeRaw(QDataStream& out, T value) {
        // Приводим к LittleEndian (так как ридер ожидает LittleEndian)
        T valLE = qToLittleEndian(value);
        // Пишем сырые байты, игнорируя метаданные QDataStream
        out.writeRawData(reinterpret_cast<const char*>(&valLE), sizeof(T));
    }

    template <typename TAttr, typename TCoord> void writeInterleavedFunc(QDataStream& out, int N) {
        for (int i = 0; i < N; ++i) {
            writeRaw<TCoord>(out, static_cast<TCoord>(i));
            writeRaw<TCoord>(out, static_cast<TCoord>(2 * i));
            writeRaw<TCoord>(out, static_cast<TCoord>(3 * i));

            // Пишем атрибут
            TAttr value = static_cast<TAttr>(i) + static_cast<TAttr>(0.5);
            writeRaw<TAttr>(out, value);
        }
    }

    template <typename TAttr, typename TCoord>
    void writeNonInterleavedFunc(QDataStream& out, int N) {
        for (int i = 0; i < N; ++i) {
            writeRaw<TCoord>(out, static_cast<TCoord>(i));
        }
        // Y
        for (int i = 0; i < N; ++i) {
            writeRaw<TCoord>(out, static_cast<TCoord>(i * 2));
        }
        // Z
        for (int i = 0; i < N; ++i) {
            writeRaw<TCoord>(out, static_cast<TCoord>(i * 3));
        }
        // Attr
        for (int i = 0; i < N; ++i) {
            writeRaw<TAttr>(out, static_cast<TAttr>(i + 0.5));
        }
    }

    // N - number of line into file
    // mode: 0 - Interleaved, 1 - NonInterleaved
    // writeHeader: particle size and timestamp
    QString CreateTestFile(int N, int mode, int attrType, int coordType, bool writeHeader = true) {
        QString file_path = "testData_tmp.bin";
        QFile   file(file_path);
        if (!file.open(QIODevice::WriteOnly)) {
            return "";
        }
        double      timestamp = 1.25;
        QDataStream out(&file);
        out.setByteOrder(QDataStream::LittleEndian);
        // out.setFloatingPointPrecision(QDataStream::DoublePrecision);
        if (writeHeader) {
            out << (int)N;
            out << (double)timestamp;
        }
        if (mode == 0) {
            // --- interleaved scheme ---
            switch (attrType) {
                case VTK_DOUBLE: {
                    if (coordType == VTK_DOUBLE)
                        writeInterleavedFunc<double, double>(out, N);
                    else
                        writeInterleavedFunc<double, float>(out, N);
                    break;
                }
                case VTK_FLOAT: {
                    if (coordType == VTK_DOUBLE)
                        writeInterleavedFunc<float, double>(out, N);
                    else
                        writeInterleavedFunc<float, float>(out, N);
                    break;
                }
                case VTK_INT: {
                    if (coordType == VTK_DOUBLE)
                        writeInterleavedFunc<int, double>(out, N);
                    else
                        writeInterleavedFunc<int, float>(out, N);
                    break;
                }
            }
        } else {
            // --- NonInterleaved scheme ---
            switch (attrType) {
                case VTK_DOUBLE: {
                    if (coordType == VTK_DOUBLE)
                        writeNonInterleavedFunc<double, double>(out, N);
                    else
                        writeNonInterleavedFunc<double, float>(out, N);

                    break;
                }
                case VTK_FLOAT: {
                    if (coordType == VTK_DOUBLE)
                        writeNonInterleavedFunc<float, double>(out, N);
                    else
                        writeNonInterleavedFunc<float, float>(out, N);

                    break;
                }
                case VTK_INT: {
                    if (coordType == VTK_DOUBLE)
                        writeNonInterleavedFunc<int, double>(out, N);
                    else
                        writeNonInterleavedFunc<int, float>(out, N);

                    break;
                }
            }
        }
        file.close();
        return file_path;
    };

    ReadScheme createScheme(bool isInterleaved, int attrType, int coordType) {
        ColumnScheme scheme;
        scheme.headerOffsetBytes = 0; // всегда читаем начиная с заголовка
        scheme.isInterleaved     = isInterleaved;

        // координаты
        ColumnMapping colCoord;
        colCoord.name               = "Coords";
        colCoord.vtkDataType        = coordType;
        colCoord.numberOfComponents = 3;
        colCoord.isCoordinate       = true;
        scheme.columnsPolicy.append(colCoord);

        ColumnMapping colAttr;
        colAttr.name               = "TestAttribute";
        colAttr.vtkDataType        = attrType;
        colAttr.numberOfComponents = 1;
        colAttr.isCoordinate       = false;
        colAttr.vtkAttributeRole   = -1; // Или vtkDataSetAttributes::SCALARS
        scheme.columnsPolicy.append(colAttr);
        return scheme;
    }
};
QTEST_MAIN(TestBINReaderRead)
#include "TestBINReaderRead.moc"