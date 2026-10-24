#include <QDataStream>
#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkType.h>

#include "Common/Enums/IOEnums.h"
#include "Common/Structures/FileSchemeStructures.h"
#include "IO/BIN/BINReader.h"

using namespace QSpace::IO;

class TestBINReaderFailures : public QObject {
    Q_OBJECT

  private slots:
    void rejectsNegativeParticleCount() { verifyRejected(makeFile("negative.bin", -1, true), validScheme()); }
    void rejectsIncompleteHeader() { verifyRejected(makeFile("header.bin", 1, false, false), validScheme()); }
    void rejectsHeaderWithoutTimestamp() { verifyRejected(makeFile("timestamp.bin", 1, false, false), validScheme()); }
    void rejectsShortPayload() { verifyRejected(makeFile("short.bin", 2, true, true, 1), validScheme()); }
    void rejectsLongPayload() { verifyRejected(makeFile("long.bin", 1, true, true, 5), validScheme()); }

    void rejectsWrongExtension() {
        QTemporaryDir dir;
        const QString path = dir.filePath("data.dat");
        makeFile(path, 1, true);
        BINReader reader;
        const auto result = reader.read(path, validScheme());
        QCOMPARE(result.status, ReadStatus::InvalidFormat);
        QVERIFY(!result.data);
    }

    void rejectsSchemeWithoutCoordinates() { verifyRejected(makeFile("no-coords.bin", 1, true), schemeWithoutCoordinates()); }

    void rejectsSeveralCoordinateColumns() {
        auto scheme = std::get<ColumnScheme>(validScheme());
        scheme.columnsPolicy.append({VTK_FLOAT, -1, 3, true, "Coordinates2"});
        verifyRejected(makeFile("two-coords.bin", 1, true), scheme);
    }

    void rejectsUnsupportedType() {
        auto scheme = std::get<ColumnScheme>(validScheme());
        scheme.columnsPolicy[1].vtkDataType = VTK_SHORT;
        verifyRejected(makeFile("type.bin", 1, true), scheme);
    }

    void rejectsEmptyAttributeName() {
        auto scheme = std::get<ColumnScheme>(validScheme());
        scheme.columnsPolicy[1].name.clear();
        verifyRejected(makeFile("name.bin", 1, true), scheme);
    }

    void rejectsZeroComponents() {
        auto scheme = std::get<ColumnScheme>(validScheme());
        scheme.columnsPolicy[1].numberOfComponents = 0;
        verifyRejected(makeFile("components.bin", 1, true), scheme);
    }

    void readsExactValuesAndMetadata_data() {
        QTest::addColumn<int>("type");
        QTest::addColumn<bool>("interleaved");
        QTest::newRow("double-interleaved") << VTK_DOUBLE << true;
        QTest::newRow("float-interleaved") << VTK_FLOAT << true;
        QTest::newRow("int-interleaved") << VTK_INT << true;
        QTest::newRow("double-columnar") << VTK_DOUBLE << false;
        QTest::newRow("float-columnar") << VTK_FLOAT << false;
        QTest::newRow("int-columnar") << VTK_INT << false;
    }

    void readsExactValuesAndMetadata() {
        QFETCH(int, type);
        QFETCH(bool, interleaved);
        QTemporaryDir dir;
        const QString path = makeFile(dir.filePath("values.bin"), 2, true, true, -1, interleaved, type);
        BINReader reader;
        const auto result = reader.read(path, schemeWithComponents(interleaved, type));
        QCOMPARE(result.status, ReadStatus::Succes);
        QCOMPARE(result.timestamp, 42.5);
        QCOMPARE(result.pointCount, 2.0);
        auto poly = vtkPolyData::SafeDownCast(result.data);
        QVERIFY(poly);
        QCOMPARE(poly->GetNumberOfPoints(), 2);
        double point[3];
        poly->GetPoint(1, point);
        QCOMPARE(point[0], 10.0);
        QCOMPARE(point[1], 20.0);
        QCOMPARE(point[2], 30.0);
        auto array = poly->GetPointData()->GetArray("Values");
        QVERIFY(array);
        QCOMPARE(array->GetNumberOfComponents(), 2);
        QCOMPARE(array->GetComponent(1, 0), 2.0);
        QCOMPARE(array->GetComponent(1, 1), 3.0);
    }

    void rejectsZeroSizeFileWhenForceMapped() {
        QTemporaryDir dir;
        const QString path = dir.filePath("empty.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        BINReader reader;
        reader.setPolicy(FilePolicy::ForceMapped);
        const auto result = reader.read(path, validScheme());
        QCOMPARE(result.status, ReadStatus::InvalidFileStructure);
        QVERIFY(!result.data);
    }

    void mapsValidFileWhenForceMapped() {
        QTemporaryDir dir;
        const QString path = makeFile(dir.filePath("mapped.bin"), 1, true);
        BINReader reader;
        reader.setPolicy(FilePolicy::ForceMapped);
        const auto result = reader.read(path, validScheme());
        QCOMPARE(result.status, ReadStatus::Succes);
        QVERIFY(result.data);
    }

  private:
    static ReadScheme validScheme() {
        ColumnScheme scheme;
        scheme.isInterleaved = true;
        scheme.columnsPolicy.append({VTK_DOUBLE, -1, 3, true, "Coordinates"});
        scheme.columnsPolicy.append({VTK_DOUBLE, -1, 2, false, "Mass"});
        return scheme;
    }

    static ReadScheme schemeWithoutCoordinates() {
        ColumnScheme scheme;
        scheme.columnsPolicy.append({VTK_DOUBLE, -1, 1, false, "Mass"});
        return scheme;
    }

    static ReadScheme schemeWithComponents(bool interleaved, int type) {
        ColumnScheme scheme;
        scheme.isInterleaved = interleaved;
        scheme.columnsPolicy.append({VTK_DOUBLE, -1, 3, true, "Coordinates"});
        scheme.columnsPolicy.append({type, -1, 2, false, "Values"});
        return scheme;
    }

    template <typename T>
    static void writeRaw(QDataStream& out, T value) {
        const T littleEndian = qToLittleEndian(value);
        out.writeRawData(reinterpret_cast<const char*>(&littleEndian), sizeof(T));
    }

    static QString makeFile(const QString& path, int n, bool payload, bool timestamp = true,
                            int payloadItems = -1, bool interleaved = true, int attrType = VTK_DOUBLE) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        QDataStream out(&file);
        out.setByteOrder(QDataStream::LittleEndian);
        out << n;
        if (timestamp) out << 42.5;
        if (payload) {
            const int count = payloadItems < 0 ? n : payloadItems;
            auto writeCoordinates = [&]() {
                for (int component = 1; component <= 3; ++component) {
                    for (int i = 0; i < count; ++i)
                        writeRaw(out, static_cast<double>(i * 10 * component));
                }
            };
            auto writeValues = [&]() {
                for (int i = 0; i < count; ++i) {
                    if (attrType == VTK_INT) {
                        writeRaw(out, i + 1);
                        writeRaw(out, i + 2);
                    } else if (attrType == VTK_FLOAT) {
                        writeRaw(out, static_cast<float>(i + 1));
                        writeRaw(out, static_cast<float>(i + 2));
                    } else {
                        writeRaw(out, static_cast<double>(i + 1));
                        writeRaw(out, static_cast<double>(i + 2));
                    }
                }
            };
            if (interleaved) {
                for (int i = 0; i < count; ++i) {
                    writeRaw(out, static_cast<double>(i * 10));
                    writeRaw(out, static_cast<double>(i * 20));
                    writeRaw(out, static_cast<double>(i * 30));
                    if (attrType == VTK_INT) {
                        writeRaw(out, i + 1);
                        writeRaw(out, i + 2);
                    } else if (attrType == VTK_FLOAT) {
                        writeRaw(out, static_cast<float>(i + 1));
                        writeRaw(out, static_cast<float>(i + 2));
                    } else {
                        writeRaw(out, static_cast<double>(i + 1));
                        writeRaw(out, static_cast<double>(i + 2));
                    }
                }
            } else {
                writeCoordinates();
                writeValues();
            }
        }
        file.close();
        return path;
    }

    static void verifyRejected(const QString& path, const ReadScheme& scheme) {
        BINReader reader;
        const auto result = reader.read(path, scheme);
        QVERIFY(result.status != ReadStatus::Succes);
        QVERIFY(!result.data);
    }
};

QTEST_APPLESS_MAIN(TestBINReaderFailures)
#include "TestBINReaderFailures.moc"




