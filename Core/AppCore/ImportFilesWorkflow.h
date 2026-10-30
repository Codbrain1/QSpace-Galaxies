#pragma once

#include "Common/Enums/IOEnums.h"
#include <QObject>
#include <QStringList>
#include <QUuid>

namespace QSpace::Core {
class DataManager;
class ObjectRegistry;
class FileSchemeRegistry;
}

namespace QSpace::Core::Controllers {

class ImportFilesWorkflow final : public QObject {
    Q_OBJECT
public:
    explicit ImportFilesWorkflow(Core::DataManager* dataManager,
                                 Core::ObjectRegistry* objectRegistry,
                                 Core::FileSchemeRegistry* fileSchemeRegistry,
                                 QObject* parent = nullptr);

    void start(const QStringList& paths,
               IO::ModelingProgrammVersion version,
               const QUuid& targetExperimentId = {});

signals:
    void taskStarted(const QUuid& taskId, const QUuid& experimentId, int totalFiles);

private:
    Core::DataManager* m_dataManager;
    Core::ObjectRegistry* m_objectRegistry;
    Core::FileSchemeRegistry* m_fileSchemeRegistry;
};

} // namespace QSpace::Core::Controllers
