#pragma once

#include "Common/Enums/IOEnums.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUuid>

namespace QSpace::Core {
class DataManager;
class ObjectRegistry;
class FileSchemeRegistry;
}

namespace QSpace::Core::Controllers {

class ImportFilesWorkflow;

class ImportExperimentWorkflow final : public QObject {
    Q_OBJECT
public:
    explicit ImportExperimentWorkflow(ImportFilesWorkflow* filesWorkflow,
                                      Core::ObjectRegistry* objectRegistry,
                                      QObject* parent = nullptr);

    void start(const QString& experimentPath, IO::ModelingProgrammVersion version);
    void start(const QStringList& filePaths,
               const QString& experimentName,
               IO::ModelingProgrammVersion version);

signals:
    void taskStarted(const QUuid& taskId, const QUuid& experimentId, int totalFiles);

private:
    ImportFilesWorkflow* m_filesWorkflow;
    Core::ObjectRegistry* m_objectRegistry;
};

} // namespace QSpace::Core::Controllers
