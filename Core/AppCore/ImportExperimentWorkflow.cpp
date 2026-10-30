#include "ImportExperimentWorkflow.h"
#include "Core/AppCore/ImportFilesWorkflow.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Common/Structures/ObjectRegistryStructures.h"
#include <QDir>
#include <QDirIterator>
#include <memory>

namespace QSpace::Core::Controllers {

ImportExperimentWorkflow::ImportExperimentWorkflow(ImportFilesWorkflow* filesWorkflow,
                                                   Core::ObjectRegistry* objectRegistry,
                                                   QObject* parent)
    : QObject(parent), m_filesWorkflow(filesWorkflow), m_objectRegistry(objectRegistry) {
    connect(m_filesWorkflow,
            &ImportFilesWorkflow::taskStarted,
            this,
            &ImportExperimentWorkflow::taskStarted);
}

void ImportExperimentWorkflow::start(const QString& experimentPath,
                                     IO::ModelingProgrammVersion version) {
    if (experimentPath.isEmpty() || !m_objectRegistry)
        return;

    const QDir dir(experimentPath);
    if (!dir.exists())
        return;

    QStringList files;
    QDirIterator iterator(experimentPath,
                          QStringList() << QStringLiteral("*.bin"),
                          QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext())
        files.append(iterator.next());

    if (files.isEmpty())
        return;

    auto experiment = std::make_shared<Core::Experiment>(dir.dirName());
    m_objectRegistry->registerExperiment(experiment);
    m_filesWorkflow->start(files, version, experiment->id);
}

void ImportExperimentWorkflow::start(const QStringList& filePaths,
                                     const QString& experimentName,
                                     IO::ModelingProgrammVersion version) {
    if (filePaths.isEmpty() || experimentName.isEmpty() || !m_objectRegistry || !m_filesWorkflow)
        return;

    auto experiment = std::make_shared<Core::Experiment>(experimentName);
    m_objectRegistry->registerExperiment(experiment);
    m_filesWorkflow->start(filePaths, version, experiment->id);
}

} // namespace QSpace::Core::Controllers
