#pragma once
#include "Common/Enums/IOEnums.h"
#include "Common/Structures/FileSchemeStructures.h"
#include "Core/TaskManager/TaskManager.h"
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qurl.h>
#include <quuid.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkSmartPointer.h>
#include "IO/ReadResult.h"
#include "Structures/ObjectRegistryStructures.h"

namespace QSpace::Core {
class DataManager : public QObject {
    Q_OBJECT
  public:
    explicit DataManager(TaskManager* tastManager, QObject* parent = nullptr);
    ~DataManager() override = default;
    /**
     * @brief Устанавливает глобальную политику (MMap/Stream) для создаваемых ридеров.
     */
    void setDefaultPolicy(IO::FilePolicy policy);
    /**
     * @brief чтение одного файла
     * создает объект ридера и читает файл в вызывающем потоке
     */
    // void importData(const QString& path, const IO::ReadScheme& scheme); //TODO: реализовать
    // чтение одного файла из вызывающего потока для анимерования данных с различной структурой

    /**
     * @brief чтение одного файла
     * создает объект ридера и читает файл в отдельном потоке,
     * Сигнал: dataReady(vtkSmartPointer<vtkDataSet>)
     */
    QUuid importDataAsync(const QString&        path,
                          const IO::ReadScheme& scheme,
                          IO::ImportRole        role = IO::ImportRole::ProjectData);
    /**
     * @brief  чтение пакета независимых файлов, каждый из которых отдельный объект
     * создает для каждого файла отдельный ридер
     * Сигнал: batchFileReady(ReadResult result)
     */
    QUuid importBatchDataAsync(const QList<IO::BatchTask>& tasks,
                               IO::ImportRole              role = IO::ImportRole::ProjectData);
    /**
     * @brief читает пакет файлов с одинаковой структурой
     * создается один ридер, но все данные независимы
     * Сигнал: batchFileReady(ReadResult result)
     */
    void importBatchIdendicalDataAsync(const QStringList&    paths,
                                       const IO::ReadScheme& scheme,
                                       IO::FileFormat        format,
                                       IO::ImportRole        role = IO::ImportRole::ProjectData);
  signals:
    // запущена операция чтения файлов
    void ioStarted(const QUuid& taskId, const QString& description, int total);
    // TODO: реализовать обработку события (нужно для прогресс бара)
    void progressChanged(const QUuid& taskId, int value, int total);
    void ioFinished(const QUuid& taskId, bool succes);

    void fileReady(const QUuid& taskId, IO::ReadResult result, IO::ImportRole role);
    void fileReadFailed(const QUuid& taskId, IO::ReadResult result, IO::ImportRole role);
    // void snapShotReady(vtkSmartPointer<vtkMultiBlockDataSet> data, const QString& name);

    void errorOccured(const QString& mes);

  private:
    TaskPriority   getPriority(qint64 fileSize);
    TaskManager*   m_taskManager;
    IO::FilePolicy m_global_policy = IO::FilePolicy::Auto;
};
} // namespace QSpace::Core