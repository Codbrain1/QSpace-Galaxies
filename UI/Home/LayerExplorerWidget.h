#pragma once
#include "Core/AppCore/AppCore.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Enums/CoreEnums.h"
#include "Models/DataTreeModel/DataTreeModel.h"
#include "SelectExperimentDialog.h"
#include "Structures/ObjectRegistryStructures.h"
#include <QListWidget>
#include <QObject>
#include <QPersistentModelIndex>
#include <QVariant>
#include <QWidget>
#include <functional>
#include <memory>
#include <qabstractitemmodel.h>
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtreewidget.h>
#include <quuid.h>
#include <qwidget.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class LayerExplorerWidget;
}
QT_END_NAMESPACE

namespace QSpace::UI {

class LayerExplorerWidget : public QWidget {
    Q_OBJECT
  public:
    struct SelectedItem {
        QUuid                      id;
        Models::DataTreeItem::Type type;
    };
    explicit LayerExplorerWidget(Core::AppCore* app, QWidget* parent = nullptr);
    ~LayerExplorerWidget();
    QList<SelectedItem> getSelectedIds() const;

  signals:
    void selectionChanged(const QList<SelectedItem>& selectedIds);
    void nodeSelectionActivated(const QUuid& id);
    void removalObjectRequested(const QUuid& id);
    void removalLayerRequested(const QUuid& id);
    void layerStructureChanged(const Models::DataTreeModel::TreeMode mode);
    void snapshotCompleteForTimeSlider(const QUuid& snapshotId);
    // сигнал вызывающийся при выборе нового эксперимента для отображения
    void targetVisualiseExperimentChanged(const QUuid& experimentId);

    // ------ отвечает за отображение меню с настройками ------
    void propertyInspectorVisibleRequested(const bool isVisible);

  private slots:

    // общие кнопки
    void handleReadSchenmeChange(int index);
    void handleStructureViewChange(int index);
    // ---------------------------------------------------------
    // @SECTION: Редактор слоев
    // ---------------------------------------------------------

    // поиск по слоям
    void handleFindLayerChange(const QString& line);
    void handleSearchResultClicked(QListWidgetItem* item);
    // ------ сортировка слоев ------
    // true -- прямой порядок A - Я, A - Z; false -- обратный порядок Я - A , Z - A
    void handleSortByAlphabetically(const bool direct);
    void handleSortByTimestemp(const bool direct);
    void handleResetSortToDefault();

    //  ------ добавление слоев в редактор слоев ------
    // порождают ноду/контейнер данных в ObjectRegistry
    void handleAddLayer(); // добавление слоя (представления)

    // обработка запросов загрузки файлов из разных вкладок UI
    void handleImportFilesRequestFromLayerEditor();
    void handleImportFileRequestFromFileExplorer();

    // void on_actionAddSnapshot_clicked();   // добавление группы для слоев
    void handleAddExperiment(); // добавление эксперимента

    void handleRemoveElement(); // удаляет объект в меню

    // ------ управление видимостью слоев ------
    void handleHideAll();        // скрывает все слои
    void handleShowAll();        // отображает все слои
    void handleHideSelected();   // скрывает только выбранные слои
    void handleShowSelected();   // отображает только выбранные слои
    void handleHideUnselected(); // скрывает невыбранные слои

    // ------ управление фильтрами слоев ------
    void handleShowLoadedLayers(const bool isChecked);        // только загруженные слои // TODO
    void handleShowUnloadedLayers(const bool isChecked);      // только незагруженные слои // TODO
    void handleFilterEquationToggled(const bool isChecked);   // фильтр по выражению // TODO
    void handleFilterEquationLineChange(const QString& line); // изменить выражение фильтра // TODO

    // ------ урпавление структурой слоев ------
    void handleExpandAll();   // разверныть все
    void handleCollapseAll(); // скрыть все

    // ---------------------------------------------------------
    // @SECTION: Проводник файлов
    // ---------------------------------------------------------

    // ------ управление корневой директорией ------
    // TODO: добавить лямбду для обработки нажатия кнопки выбора папки
    void handleRootPathChange(const QString& line); // MINOR: возможно будет работать некорреткно
    //  ------ Добавление/удаление данных ------
    void handleCollapseAllFiles();

    // поиск по файлам
    void handleFindFile(const QString& line); // TODO

    void handleNodeSelected();
    void handleShowCustomContexMenuForTreeViewElement(const QPoint& pos);

    // void onItemChanged(QTreeWidgetItem* item, int col);

    void handleShowCustomContextMenuForFile(const QPoint& pos);

  private:
    void setupSlots();        // TODO
    void setupToolButtons();  // TODO
    void setupFileExplorer(); // TODO
    void sortTreeHierarchyInternal(QTreeView*                                                  tree,
                                   std::function<bool(const QModelIndex&, const QModelIndex&)> comparator,
                                   Qt::SortOrder order = Qt::AscendingOrder);

    // загрузка данных и добавление записи в реестр
    void selectAndImportFilesInternal(const SelectExperimentDialogResult& result,
                                      const QStringList&                  filePaths);
    std::optional<SelectExperimentDialogResult>
    selectExperimentDialogInternal(); // открывает диалог выбора эксперимента для добавления файлов

    Ui::LayerExplorerWidget* ui;
    QListWidget*             m_searchPopup = nullptr; // отображает записи по поиску
    Core::AppCore*           m_app;
    Models::DataTreeModel*   m_treeModel = nullptr;

    // по умолчанию равен корню диска
    QString                       m_root_path;
    Core::ModelingProgrammVersion m_currentVersion = Core::ModelingProgrammVersion::V2;

    void setCheckStateRecursiveInternal(const QModelIndex& parentIndex, Qt::CheckState state);
    void setCheckStateUnselectedRecursiveInternal(const QModelIndex&       parentIndex,
                                                  const QSet<QModelIndex>& selectedIndexes,
                                                  Qt::CheckState           state);

    void showCustomContextMenuForExperimentInternal(QMenu* menu, const QModelIndex& index);     // TODO
    void showCustomContextMenuForSnapshotInternal(QMenu* menu, const QModelIndex& index);       // TODO
    void showCustomContextMenuForDataNodeInternal(QMenu* menu, const QModelIndex& index);       // TODO
    void showCustomContextMenuForComponentGroupInternal(QMenu* menu, const QModelIndex& index); // TODO
    void showCustomContextMenuForLayerInternal(QMenu* menu, const QModelIndex& index);          // TODO

    void    searchTreeRecursively(const QModelIndex&  parent,
                                  const QString&      text,
                                  QAbstractItemModel* model,
                                  QList<QModelIndex>& results);
    QString buildItemContextString(const QModelIndex& index, QAbstractItemModel* model);
};
} // namespace QSpace::UI