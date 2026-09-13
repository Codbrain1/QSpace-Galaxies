#include "LayerExplorerWidget.h"
#include "Common/Logger/Logger.h"
#include "Core/AppCore/AppCore.h"
#include "Core/AppCore/VideoController.h"
#include "Core/AppCore/ViewController.h"
#include "Core/ObjectRegistry/ObjectRegistry.h"
#include "Enums/CoreEnums.h"
#include "Models/DataTreeModel/DataTreeModel.h"
#include "SelectExperimentDialog.h"
#include "Structures/ObjectRegistryStructures.h"
#include "ui_LayerExplorerWidget.h"
#include <Core/AppCore/DataController.h>
#include <QActionGroup>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QTreeView>

#include <memory>
#include <qabstractitemmodel.h>
#include <qabstractspinbox.h>
#include <qaction.h>
#include <qcombobox.h>
#include <qcontainerfwd.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qheaderview.h>
#include <qlineedit.h>
#include <qloggingcategory.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qtreeView.h>
#include <qtreeview.h>
#include <quuid.h>

namespace QSpace::UI {
class CustomSortProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
    std::function<bool(const QModelIndex&, const QModelIndex&)> m_comparator;

  public:
    CustomSortProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {
    }

    void setComparator(std::function<bool(const QModelIndex&, const QModelIndex&)> comp) {
        m_comparator = comp;
        invalidate(); // Заставляем модель пересортироваться
    }

  protected:
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override {
        if (m_comparator) {
            return m_comparator(source_left, source_right);
        }
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }
};

LayerExplorerWidget::LayerExplorerWidget(Core::AppCore* app, QWidget* parent)
    : QWidget(parent), ui(new Ui::LayerExplorerWidget), m_app(app) {
    ui->setupUi(this);

    // 1. Подключаем готовую модель из AppCore к нашему QTreeView (ui->treeView_Layers)
    if (m_app && m_app->dataTreeModel()) {
        auto* actualDataModel = m_app->dataTreeModel(); // Получаем прямой указатель

        auto* proxyModel = new CustomSortProxyModel(this);
        proxyModel->setSourceModel(actualDataModel);
        proxyModel->setFilterKeyColumn(0); // Фильтруем по первой колонке (название слоя)
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setDynamicSortFilter(true);

        // В QTreeView устанавливаем ИМЕННО прокси-модель
        ui->treeView_Layers->setModel(proxyModel);

        // Сохраняем указатель в поле класса (если m_treeModel объявлен в хедере),
        m_treeModel = actualDataModel;

        // даем команду на первичное построение дерева. Сигналы гарантированно дойдут до UI.
        m_treeModel->rebuildTree();
        handleExpandAll();
    }
    // Настройки отображения
    ui->treeView_Layers->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView_Layers->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->treeView_Layers->setAnimated(true);

    QHeaderView* header = ui->treeView_Layers->header();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // выбор версии программы моделирвоания для загрузки файлов
    ui->comboBox_fileStructure->setItemData(0, static_cast<int>(Core::ModelingProgrammVersion::V2));
    ui->comboBox_fileStructure->setItemData(1, static_cast<int>(Core::ModelingProgrammVersion::V2_2));
    ui->comboBox_fileStructure->setItemData(2, static_cast<int>(Core::ModelingProgrammVersion::V2_3));

    // В конструкторе LayerExplorerWidget, после ui->setupUi(this);
    m_searchPopup = new QListWidget(this);
    // Делаем его плавающим окном-подсказкой без рамки
    m_searchPopup->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    // Запрещаем окну отбирать фокус клавиатуры у строки поиска
    m_searchPopup->setAttribute(Qt::WA_ShowWithoutActivating);
    m_searchPopup->setFocusPolicy(Qt::NoFocus);
    m_searchPopup->hide();

    setupToolButtons();
    setupSlots();
    setupFileExplorer();
}

LayerExplorerWidget::~LayerExplorerWidget() {
    delete ui;
}

QList<LayerExplorerWidget::SelectedItem> LayerExplorerWidget::getSelectedIds() const {
    QList<LayerExplorerWidget::SelectedItem> ids;
    auto                                     indexes = ui->treeView_Layers->selectionModel()->selectedRows(0);

    for (const QModelIndex& index : indexes) {
        QUuid id = index.data(Models::DataTreeModel::CustomRoles::IdRole).toUuid();
        if (!id.isNull()) {
            // Безопасно достаем тип через механизм ролей Qt (прокси обработает это сам)
            int  typeInt = index.data(Models::DataTreeModel::CustomRoles::TypeRole).toInt();
            auto type    = static_cast<Models::DataTreeItem::Type>(typeInt);

            ids.append({id, type});
        }
    }
    return ids;
}
void LayerExplorerWidget::setupSlots() {
    // --------- Менеджер Файлов ---------
    // изменение корневой директории для проводника файлов
    connect(ui->lineEdit_root_path,
            &QLineEdit::textChanged,
            this,
            &LayerExplorerWidget::handleRootPathChange);
    // выбор корневой директории для проводника файлов
    connect(ui->pushButton_change_root, &QPushButton::clicked, this, [this]() {
        const QString dirPath =
            QFileDialog::getExistingDirectory(this, tr("Пожалуйста выберите директорию"), m_root_path);
        handleRootPathChange(dirPath);
    });

    // --------- Менеджер слоев ---------
    // сообщает ядру об активации ноды
    connect(this,
            &LayerExplorerWidget::nodeSelectionActivated,
            m_app->dataController(),
            &QSpace::Core::Controllers::DataController::onNodeSelectionActivated,
            Qt::QueuedConnection);
    // отвечает за отображение меню с настройками
    connect(ui->toolButton_VisiblePropertyInspector,
            &QToolButton::toggled,
            this,
            &LayerExplorerWidget::propertyInspectorVisibleRequested);
    // изменение структуры отображения для проводника данных (слоев)
    connect(ui->comboBox_structureView,
            &QComboBox::currentIndexChanged,
            this,
            &LayerExplorerWidget::handleStructureViewChange);

    // изменение схемы чтения данных
    connect(ui->comboBox_fileStructure,
            &QComboBox::currentIndexChanged,
            this,
            &LayerExplorerWidget::handleReadSchenmeChange);
    // поиск в проводнике слоев
    connect(ui->lineEdit_findLayer,
            &QLineEdit::textChanged,
            this,
            &LayerExplorerWidget::handleFindLayerChange);
    // вызов отрисовки меню по нажатию ПКМ
    connect(ui->treeView_Layers,
            &QTreeView::customContextMenuRequested,
            this,
            &LayerExplorerWidget::handleShowCustomContexMenuForTreeViewElement);
    connect(ui->treeView_Layers, &QTreeView::expanded, this, [this](const QModelIndex& index) {
        ui->treeView_Layers->header()->resizeSection(0, ui->treeView_Layers->sizeHintForRow(0));
    });
    connect(ui->treeView_Layers, &QTreeView::collapsed, this, [this](const QModelIndex& index) {
        ui->treeView_Layers->header()->resizeSection(0, ui->treeView_Layers->sizeHintForRow(0));
    });
    // отправляет сигнал в MainWindow о том какая сейчас выбрана запись, что позволяет на лету менять вид
    // для меню настроек
    connect(ui->treeView_Layers->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &LayerExplorerWidget::handleNodeSelected);

    // удаление выбранных элементов
    connect(ui->toolButton_RemoveElement,
            &QToolButton::clicked,
            this,
            &LayerExplorerWidget::handleRemoveElement);

    if (m_app && m_app->dataTreeModel()) {
        connect(this,
                &LayerExplorerWidget::removalObjectRequested,
                m_app->dataController(),
                &Core::Controllers::DataController::removeNodeObject,
                Qt::QueuedConnection);
        connect(this,
                &LayerExplorerWidget::removalLayerRequested,
                m_app->dataController(),
                &Core::Controllers::DataController::removeLayer,
                Qt::QueuedConnection);
    }
    connect(ui->toolButton_CollapseAll, &QToolButton::clicked, this, &LayerExplorerWidget::handleCollapseAll);
    connect(ui->toolButton_ExpandAll, &QToolButton::clicked, this, &LayerExplorerWidget::handleExpandAll);
    // Подключаем клик по элементу списка
    connect(m_searchPopup, &QListWidget::itemClicked, this, &LayerExplorerWidget::handleSearchResultClicked);

    // изменение отображаемого эксперимента
    connect(this,
            &LayerExplorerWidget::targetVisualiseExperimentChanged,
            m_app->videoController(),
            &QSpace::Core::Controllers::VideoController::handleTargetExperimentChange);
    connect(this,
            &LayerExplorerWidget::snapshotCompleteForTimeSlider,
            m_app->videoController(),
            &QSpace::Core::Controllers::VideoController::handleFixedEtalonSnapshot);
}

void LayerExplorerWidget::setupToolButtons() {
    auto* layout = ui->horizontalLayout_3;
    for (int i = 0; i < layout->count(); ++i) {
        auto* toolbutton = qobject_cast<QToolButton*>(layout->itemAt(i)->widget());
        if (toolbutton) {
            toolbutton->setFixedSize(30, 30);
        }
    }
    // ---------------------------------------------------------
    // @SECTION: сортировка слоев
    // ---------------------------------------------------------

    QMenu* sortingMenu = new QMenu(
        this); // TODO: при первом запуске кнопка сортировать в обратном порядке не должна быть включена

    QAction* sortByAlphabetically = new QAction(tr("Сортирвать по алфавиту"));
    QAction* sortByTimestemp      = new QAction(tr("Сортирвать по времени"));
    QAction* sortOrderInverted    = new QAction(tr("Сортирвать в обратном порядке"));
    QAction* sortByNone           = new QAction(tr("Без сортировки"));

    sortByAlphabetically->setCheckable(true);
    sortByAlphabetically->setChecked(false);

    sortByTimestemp->setCheckable(true);
    sortByTimestemp->setChecked(false);

    sortOrderInverted->setCheckable(true);
    sortOrderInverted->setChecked(false);

    sortByNone->setChecked(true);
    sortByNone->setCheckable(true);

    QActionGroup* criteriaGroup = new QActionGroup(this);
    criteriaGroup->addAction(sortByAlphabetically);
    criteriaGroup->addAction(sortByTimestemp);
    criteriaGroup->addAction(sortByNone);
    criteriaGroup->setExclusive(true);

    sortingMenu->addAction(sortByAlphabetically);
    sortingMenu->addAction(sortByTimestemp);
    sortingMenu->addAction(sortByNone);
    sortingMenu->addSeparator();
    sortingMenu->addAction(sortOrderInverted);

    ui->toolButton_SortingLayers->setMenu(sortingMenu);

    connect(sortByAlphabetically, &QAction::triggered, this, [this, sortOrderInverted]() {
        sortOrderInverted->setEnabled(true);
        handleSortByAlphabetically(!sortOrderInverted->isChecked());
    });
    connect(sortByTimestemp, &QAction::triggered, this, [this, sortOrderInverted]() {
        sortOrderInverted->setEnabled(true);
        handleSortByTimestemp(!sortOrderInverted->isChecked());
    });
    connect(sortByNone, &QAction::triggered, this, [this, sortOrderInverted]() {
        sortOrderInverted->setEnabled(false);
        handleResetSortToDefault();
    });
    connect(sortOrderInverted, &QAction::triggered, this, [this, sortByAlphabetically, sortOrderInverted]() {
        if (sortByAlphabetically->isChecked()) {
            handleSortByAlphabetically(!sortOrderInverted->isChecked());
        } else {
            handleSortByTimestemp(!sortOrderInverted->isChecked());
        }
    });

    // ---------------------------------------------------------
    // @SECTION: Добавление элемента
    // ---------------------------------------------------------
    QMenu* addElementMenu = new QMenu(this);

    QAction* actionAddLayer      = new QAction(tr("Добавить слой (представление)"));
    QAction* actionImportFiles   = new QAction(tr("Импортировать файлы с данными"));
    QAction* actionAddExperiment = new QAction(tr("Импортировать эксперимент"));

    addElementMenu->addAction(actionAddLayer);
    addElementMenu->addAction(actionImportFiles);
    addElementMenu->addAction(actionAddExperiment);

    ui->toolButton_AddElement->setMenu(addElementMenu);
    connect(actionAddExperiment, &QAction::triggered, this, &LayerExplorerWidget::handleAddExperiment);
    connect(actionImportFiles,
            &QAction::triggered,
            this,
            &LayerExplorerWidget::handleImportFilesRequestFromLayerEditor);
    connect(actionAddLayer, &QAction::triggered, this, &LayerExplorerWidget::handleAddLayer);
}

void LayerExplorerWidget::setupFileExplorer() {
    // 1. Создаем готовую модель файловой системы
    QFileSystemModel* fileModel = new QFileSystemModel(this);

    // Указываем корневой путь (модель начнет асинхронно сканировать диск отсюда)
    // Для теста можно жестко зашить, а в будущем брать из настроек проекта
    m_root_path = QCoreApplication::applicationDirPath();
    // На всякий случай проверяем (хотя папка запуска обязана существовать)
    if (!QDir(m_root_path).exists()) {
        m_root_path = QDir::currentPath(); // Альтернативный вариант (рабочая директория)
    }
    fileModel->setRootPath(m_root_path);

    // 2. Настраиваем фильтр файлов (отображаем только нужные форматы)
    fileModel->setNameFilters(QStringList() << "*.bin" << "*.hdf5" << "*.csv" << "*.dat");
    // Если false -> файлы, не прошедшие фильтр, будут скрыты (а не просто задизейблены)
    fileModel->setNameFilterDisables(false);

    // По умолчанию модель показывает и папки. Если нужно скрыть скрытые/системные файлы:
    fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    // 3. Связываем модель с отображением (QTreeView)
    ui->treeViewFiles->setModel(fileModel);

    // Важно: говорим TreeView отображать дерево именно с нашей корневой папки,
    // иначе он покажет весь компьютер (Мой компьютер, Диск C, Диск D и т.д.)
    ui->treeViewFiles->setRootIndex(fileModel->index(m_root_path));

    // 4. Тонкая настройка внешнего вида (Кастомизация под ваш интерфейс)
    // Скрываем ненужные колонки, если вам нужно только имя файла:
    ui->treeViewFiles->setColumnHidden(1, true); // Скрыть колонку "Размер"
    ui->treeViewFiles->setColumnHidden(2, true); // Скрыть колонку "Тип"
    ui->treeViewFiles->setColumnHidden(3, true); // Скрыть колонку "Дата изменения"

    // Растягиваем оставшуюся колонку с именем на всю ширину панели
    ui->treeViewFiles->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Разрешаем раскрывать папки по клику
    ui->treeViewFiles->setAnimated(true);
    ui->treeViewFiles->setSortingEnabled(true);                     // Разрешаем сортировку по алфавиту
    ui->treeViewFiles->setContextMenuPolicy(Qt::CustomContextMenu); // Для будущего контекстного меню

    connect(ui->treeViewFiles,
            &QTreeView::customContextMenuRequested,
            this,
            &LayerExplorerWidget::handleShowCustomContextMenuForFile);
}

void LayerExplorerWidget::handleReadSchenmeChange(int index) {
    if (index < 0)
        return;
    m_currentVersion =
        static_cast<Core::ModelingProgrammVersion>(ui->comboBox_fileStructure->itemData(index).toInt());
}

void LayerExplorerWidget::handleStructureViewChange(int index) {
    if (!m_app || !m_app->dataTreeModel() || !ui->treeView_Layers) {
        return;
    }

    auto* treeModel = m_app->dataTreeModel();
    auto  mode      = static_cast<Models::DataTreeModel::TreeMode>(index);

    // 1. Переключаем режим внутри модели.
    // Модель сама вызовет beginResetModel/endResetModel, View обновится автоматически.
    treeModel->setTreeMode(mode);
    emit layerStructureChanged(mode);
    // 2. Управляем раскрытием дерева (Умный UX)
    // Запрашиваем актуальную модель у View (на случай, если используется QSortFilterProxyModel)
    auto* currentViewModel = ui->treeView_Layers->model();
    if (!currentViewModel) {
        return;
    }

    // Блокируем отрисовку на время массового изменения состояния веток
    ui->treeView_Layers->updatesEnabled(); // Альтернатива blockSignals для UI

    // Сначала сворачиваем всё, чтобы убрать артефакты от предыдущего режима
    ui->treeView_Layers->collapseAll();

    // Обходим дерево на нужную нам глубину (до 2-го уровня включительно)
    for (int i = 0; i < currentViewModel->rowCount(); ++i) {
        // Уровень 1: Эксперименты (раскрываем всегда)
        QModelIndex expIndex = currentViewModel->index(i, 0);
        ui->treeView_Layers->setExpanded(expIndex, true);

        // Уровень 2: Снапшоты или Группы компонент
        int childCount = currentViewModel->rowCount(expIndex);
        for (int j = 0; j < childCount; ++j) {
            QModelIndex childIndex = currentViewModel->index(j, 0, expIndex);

            // Раскрываем Снапшот во временном виде ИЛИ Группу (Газ/Звезды) в плоском виде
            ui->treeView_Layers->setExpanded(childIndex, true);

            // Ноды (файлы данных) и Слои внутри них остаются свернутыми!
        }
    }
    ui->treeView_Layers->header()->resizeSection(0, ui->treeView_Layers->sizeHintForRow(0));

    ui->treeView_Layers->setUpdatesEnabled(true);
}

void LayerExplorerWidget::handleFindLayerChange(const QString& line) {
    auto* proxy = qobject_cast<QSortFilterProxyModel*>(ui->treeView_Layers->model());
    if (!proxy)
        return;

    // 1. Принудительно отключаем фильтрацию самого дерева, чтобы оно оставалось полным
    proxy->setFilterFixedString("");

    // 2. Если строка пустая или слишком короткая — прячем окно
    if (line.trimmed().isEmpty()) {
        m_searchPopup->hide();
        return;
    }

    QAbstractItemModel* sourceModel = proxy->sourceModel();
    if (!sourceModel)
        return;

    m_searchPopup->clear();
    QList<QModelIndex> results;

    // 3. Запускаем рекурсивный поиск по ИСХОДНОЙ модели
    searchTreeRecursively(QModelIndex(), line, sourceModel, results);

    if (results.isEmpty()) {
        m_searchPopup->hide();
        return;
    }

    // 4. Наполняем QListWidget результатами
    for (const QModelIndex& idx : results) {
        // Формируем красивую строку пути, например: "Эксперимент 1 -> Снапшот -> Звезды"
        QString displayText = buildItemContextString(idx, sourceModel);

        QListWidgetItem* listItem = new QListWidgetItem(displayText);

        // ВАЖНО: Сохраняем QPersistentModelIndex внутри элемента списка.
        // Это безопасно сохранит ссылку на ноду дерева, даже если оно слегка изменится.
        listItem->setData(Qt::UserRole, QVariant::fromValue(QPersistentModelIndex(idx)));

        m_searchPopup->addItem(listItem);
    }

    // 5. Позиционируем окно ровно под QLineEdit и задаем ему такую же ширину
    QPoint pos = ui->lineEdit_findLayer->mapToGlobal(QPoint(0, ui->lineEdit_findLayer->height()));
    m_searchPopup->move(pos);
    m_searchPopup->setFixedWidth(ui->lineEdit_findLayer->width());

    // Ограничиваем высоту, если результатов слишком много (макс 10 видимых строк)
    int popupHeight = qMin(m_searchPopup->sizeHintForRow(0) * results.size() + 5, 200);
    m_searchPopup->setFixedHeight(popupHeight);

    m_searchPopup->show();
}

// --- Обработка клика по элементу во всплывающем окне ---
void LayerExplorerWidget::handleSearchResultClicked(QListWidgetItem* item) {
    if (!item)
        return;

    // Прячем меню
    m_searchPopup->hide();

    // Достаем сохраненный индекс исходной модели
    QVariant data = item->data(Qt::UserRole);
    if (!data.canConvert<QPersistentModelIndex>())
        return;

    QPersistentModelIndex persistentIdx = data.value<QPersistentModelIndex>();
    if (!persistentIdx.isValid())
        return;

    QModelIndex sourceIdx = persistentIdx; // Приводим к обычному индексу

    auto* proxy = qobject_cast<QSortFilterProxyModel*>(ui->treeView_Layers->model());
    if (!proxy)
        return;

    // Конвертируем индекс исходной модели в индекс прокси-модели (с учетом текущей сортировки дерева)
    QModelIndex proxyIdx = proxy->mapFromSource(sourceIdx);

    if (proxyIdx.isValid()) {
        // Прокручиваем к элементу (это автоматически раскроет все родительские папки!)
        ui->treeView_Layers->scrollTo(proxyIdx, QAbstractItemView::PositionAtCenter);

        // Выделяем строку
        ui->treeView_Layers->selectionModel()->select(proxyIdx,
                                                      QItemSelectionModel::ClearAndSelect |
                                                          QItemSelectionModel::Rows);

        // Передаем фокус
        ui->treeView_Layers->setCurrentIndex(proxyIdx);
        ui->treeView_Layers->setFocus();
    }
}

// --- Сортировка внутри экспериментов по алфавиту ---
void LayerExplorerWidget::handleSortByAlphabetically(const bool direct) {
    // Определяем направление сортировки для Qt
    Qt::SortOrder order = direct ? Qt::AscendingOrder : Qt::DescendingOrder;

    sortTreeHierarchyInternal(
        ui->treeView_Layers,
        [](const QModelIndex& a, const QModelIndex& b) {
            // Компаратор теперь всегда работает ТОЛЬКО по возрастанию (<)
            return QString::compare(a.data(Qt::DisplayRole).toString(),
                                    b.data(Qt::DisplayRole).toString(),
                                    Qt::CaseInsensitive) < 0;
        },
        order);
}

// --- Сортировка внутри экспериментов по физическому моменту времени (Timestamp) ---
void LayerExplorerWidget::handleSortByTimestemp(const bool direct) {
    Qt::SortOrder order = direct ? Qt::AscendingOrder : Qt::DescendingOrder;

    sortTreeHierarchyInternal(
        ui->treeView_Layers,
        [](const QModelIndex& a, const QModelIndex& b) {
            double timeA = a.data(Models::DataTreeModel::CustomRoles::TimestampRole).toDouble();
            double timeB = b.data(Models::DataTreeModel::CustomRoles::TimestampRole).toDouble();

            // ЗАЩИТА: Если элементы имеют одинаковое время (например, слои внутри одной ноды),
            // сортируем их по исходному индексу строки, чтобы они не перемешивались случайно
            if (timeA == timeB) {
                return a.row() < b.row();
            }

            return timeA < timeB;
        },
        order);
}
// --- Сброс к хронологическому порядку ---
void LayerExplorerWidget::handleResetSortToDefault() {
    // Передаем nullptr, функция сама выключит сортировку
    sortTreeHierarchyInternal(ui->treeView_Layers, nullptr, Qt::AscendingOrder);
}

void LayerExplorerWidget::sortTreeHierarchyInternal(
    QTreeView*                                                  tree,
    std::function<bool(const QModelIndex&, const QModelIndex&)> comparator,
    Qt::SortOrder                                               order) { // Принимаем order
    if (!tree)
        return;

    auto* proxy = qobject_cast<CustomSortProxyModel*>(tree->model());
    if (!proxy)
        return;

    tree->setUpdatesEnabled(false);

    // Передаем лямбду в прокси (будет использована в lessThan)
    proxy->setComparator(comparator);

    if (!comparator) {
        // -1 отключает сортировку и возвращает исходный порядок sourceModel
        proxy->sort(-1);
    } else {
        // Передаем нужный порядок! Если order == DescendingOrder,
        // Qt сам перевернет результаты вашего компаратора.
        proxy->sort(0, order);
    }

    tree->setUpdatesEnabled(true);
}

void LayerExplorerWidget::handleShowCustomContexMenuForTreeViewElement(const QPoint& pos) {
    if (!ui || !ui->treeView_Layers) {
        return;
    }
    // 1. Получаем индекс элемента (прокси-индекс) под курсором мыши
    QModelIndex proxyIndex = ui->treeView_Layers->indexAt(pos);

    // Если кликнули в пустую область (не по элементу), можно либо выйти,
    // либо показать какое-то базовое меню (например, "Добавить эксперимент")
    if (!proxyIndex.isValid()) {
        qCWarning(LogUI) << "proxy index is invalid for position: " << pos;
        return;
    }
    auto*       proxy       = qobject_cast<QSortFilterProxyModel*>(ui->treeView_Layers->model());
    QModelIndex sourceIndex = proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;
    auto*       item        = static_cast<Models::DataTreeItem*>(sourceIndex.internalPointer());
    if (!item) {
        return;
    }
    // 4. Создаем контекстное меню
    QMenu contextMenu(this);
    // 5. Вызываем соответствующий метод-помощник в зависимости от типа узла
    // Передаем в методы само меню (чтобы наполнить его action'ами) и sourceIndex (чтобы знать, для кого меню)
    switch (item->type()) {
        case Models::DataTreeItem::Experiment:
            showCustomContextMenuForExperimentInternal(&contextMenu, sourceIndex);
            break;
        case Models::DataTreeItem::Snapshot:
            showCustomContextMenuForSnapshotInternal(&contextMenu, sourceIndex);
            break;
        case Models::DataTreeItem::ComponentGroup:
            showCustomContextMenuForComponentGroupInternal(&contextMenu, sourceIndex);
            break;
        case Models::DataTreeItem::DataNode:
            showCustomContextMenuForDataNodeInternal(&contextMenu, sourceIndex);
            break;
        case Models::DataTreeItem::LayerItem:
            showCustomContextMenuForLayerInternal(&contextMenu, sourceIndex);
            break;
        default:
            break; // Для Root или неизвестных типов меню не показываем
    }
    if (!contextMenu.isEmpty()) {
        // Обязательно конвертируем локальные координаты QTreeView в глобальные координаты экрана
        contextMenu.exec(ui->treeView_Layers->viewport()->mapToGlobal(pos));
    }
}

void LayerExplorerWidget::showCustomContextMenuForExperimentInternal(QMenu* menu, const QModelIndex& index) {
    QUuid    experimentId = index.data(Models::DataTreeModel::CustomRoles::IdRole).toUuid();
    QAction* deleteAction = menu->addAction(tr("Удалить эксперимент"));

    connect(deleteAction, &QAction::triggered, this, [this, experimentId]() {
        emit removalObjectRequested(experimentId);
    });

    auto mode = static_cast<Models::DataTreeModel::TreeMode>(ui->comboBox_structureView->currentIndex());
    if (mode == Models::DataTreeModel::TreeMode::SnapShotView) {
        QAction* selectForVisualizeAction = menu->addAction(tr("Выбрать для исследования"));
        connect(selectForVisualizeAction, &QAction::triggered, this, [this, experimentId]() {
            emit targetVisualiseExperimentChanged(experimentId);
        });
    }
}
void LayerExplorerWidget::showCustomContextMenuForSnapshotInternal(QMenu* menu, const QModelIndex& index) {
    QUuid snapshotId = index.data(Models::DataTreeModel::CustomRoles::IdRole).toUuid();

    QAction* completeForTimeSliderAction = menu->addAction(tr("Применить для плеера кадров"));
    QAction* deleteAction                = menu->addAction(tr("Удалить снимок"));

    connect(deleteAction, &QAction::triggered, this, [this, snapshotId]() {
        emit removalObjectRequested(snapshotId);
    });
    connect(completeForTimeSliderAction, &QAction::triggered, this, [this, snapshotId] {
        emit snapshotCompleteForTimeSlider(snapshotId);
    });
}
void LayerExplorerWidget::showCustomContextMenuForDataNodeInternal(QMenu* menu, const QModelIndex& index) {
    QUuid    dataNodeId     = index.data(Models::DataTreeModel::CustomRoles::IdRole).toUuid();
    QAction* addLayerAction = menu->addAction(tr("Добавить слой"));
    QAction* loadDataAction = menu->addAction(tr("Загрузить данные"));
    QAction* deleteAction   = menu->addAction(tr("Удалить компоненту"));

    connect(deleteAction, &QAction::triggered, this, [this, dataNodeId]() {
        emit removalObjectRequested(dataNodeId);
    });
    connect(loadDataAction, &QAction::triggered, this, [this, dataNodeId]() {
        emit nodeSelectionActivated(dataNodeId);
    });
    connect(addLayerAction, &QAction::triggered, this, [this, dataNodeId]() {
        const auto dataController = m_app->dataController();
        if (dataController) {
            dataController->createLayerForNode(dataNodeId);
        }
    });
}
void LayerExplorerWidget::showCustomContextMenuForComponentGroupInternal(QMenu*             menu,
                                                                         const QModelIndex& index) {
    QUuid componentId = index.data(Models::DataTreeModel::CustomRoles::IdRole).toUuid();
}

void LayerExplorerWidget::showCustomContextMenuForLayerInternal(QMenu* menu, const QModelIndex& index) {
    // Получаем UUID конкретного слоя
    QUuid layerId = index.data(Models::DataTreeModel::CustomRoles::IdRole).toUuid();

    // Создаем Action
    QAction* deleteAction = menu->addAction(tr("Удалить слой"));
    menu->addSeparator();
    QAction* renameAction = menu->addAction(tr("Переименовать"));

    connect(renameAction, &QAction::triggered, this, [this, index]() { ui->treeView_Layers->edit(index); });

    // Подключаем логику удаления
    connect(deleteAction, &QAction::triggered, this, [this, layerId]() {
        // Здесь обращаемся к вашему LayerManager для удаления
        emit removalLayerRequested(layerId);
    });
}

void LayerExplorerWidget::handleAddLayer() {
    // 1. Проверяем инициализацию ядра и представления дерева слоев
    if (!m_app || !ui || !ui->treeView_Layers) {
        return;
    }

    // 2. Получаем текущий выделенный индекс из представления (это индекс прокси-модели)
    QModelIndex proxyIndex = ui->treeView_Layers->currentIndex();
    if (!proxyIndex.isValid()) {
        QMessageBox::information(this,
                                 tr("Внимание"),
                                 tr("Чтобы создать представление данных выберите их в проводнике слоев"));
        // Если ничего не выбрано, выходим
        return;
    }

    // 3. Преобразуем прокси-индекс в индекс оригинальной DataTreeModel
    auto*       proxy       = qobject_cast<QSortFilterProxyModel*>(ui->treeView_Layers->model());
    QModelIndex sourceIndex = proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;

    // 4. Извлекаем UUID ноды данных, используя вашу кастомную роль
    QUuid nodeId = sourceIndex.data(Models::DataTreeModel::CustomRoles::IdRole).toUuid();
    if (nodeId.isNull()) {
        // Кликнули по элементу, у которого нет UUID (например, по пустой группе или корню)
        return;
    }

    // 5. Делегируем бизнес-логику в DataController ядра системы
    const auto dataController = m_app->dataController();
    if (dataController) {
        dataController->createLayerForNode(nodeId);
    }
}

void LayerExplorerWidget::handleImportFilesRequestFromLayerEditor() {
    auto result = selectExperimentDialogInternal();
    if (!result.has_value()) {
        return;
    }

    // 1. Создаем объект диалога вместо вызова статического метода
    QFileDialog dialog(this,
                       tr("Выберите файлы с данными"),
                       m_root_path,
                       tr("Файлы симуляции (*.bin *.hdf5 *.csv);;Все файлы (*.*)"));

    // 2. Настраиваем режим выбора нескольких существующих файлов
    dialog.setFileMode(QFileDialog::ExistingFiles);

    // 3. Запускаем диалог в модальном режиме
    if (dialog.exec() == QDialog::Accepted) {
        // Пользователь нажал "Открыть" и выбрал файлы
        QStringList filePaths = dialog.selectedFiles();
        if (!filePaths.isEmpty()) {
            selectAndImportFilesInternal(result.value(), filePaths);
        }
    }
    // Пользователь закрыл окно или нажал "Отмена"
    // Получаем путь к папке, в которой он находился в этот момент:
    QString lastDirectory = dialog.directory().absolutePath();
    qCDebug(LogUI) << "Пользователь отменил выбор, но находился в папке:" << lastDirectory;
    m_root_path = lastDirectory;
}

void LayerExplorerWidget::handleImportFileRequestFromFileExplorer() {
    QModelIndex currentIndex = ui->treeViewFiles->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Внимание"), tr("Пожалуйста, выберите файл для импорта"));
        return;
    }

    QString filePath = currentIndex.data(QFileSystemModel::FilePathRole).toString();
    if (filePath.isEmpty()) {
        qCWarning(LogUI) << "Selected File path is empty.";
        return;
    }

    auto* dataController = m_app->dataController();
    if (!dataController) {
        qCCritical(LogUI) << "DataController pointer is segmentation fault!";
        return;
    }
    auto existingNodeId_opt = dataController->getNodeIdByFilePath(filePath);
    if (existingNodeId_opt.has_value()) {
        emit nodeSelectionActivated(existingNodeId_opt.value());
        qCInfo(LogIO) << "Data file added at existing node" << existingNodeId_opt.value();
        return;
    }
    auto result = selectExperimentDialogInternal();
    if (result.has_value()) {
        selectAndImportFilesInternal(result.value(), {filePath});
    }
}

std::optional<SelectExperimentDialogResult> LayerExplorerWidget::selectExperimentDialogInternal() {
    auto experiments =
        m_app->dataController()->getExperiments(); // Получаем список экспериментов из контроллера

    SelectExperimentDialog dialog(experiments, this);
    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt; // Пользователь отменил выбор
    }
    auto result = dialog.getResult();
    return result;
}

void LayerExplorerWidget::selectAndImportFilesInternal(const SelectExperimentDialogResult& result,
                                                       const QStringList&                  filePaths) {
    if (filePaths.isEmpty())
        return;

    if (result.isNewExperiment) {
        m_app->dataController()->importExperiment(filePaths, result.newExperimentName, m_currentVersion);
    } else {
        m_app->dataController()->importFiles(filePaths, m_currentVersion, result.exisitingExperimentId);
    }
}

void LayerExplorerWidget::handleAddExperiment() {
    // Запрашиваем директорию эксперимента
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Выберите папку эксперимента"), m_root_path);
    if (dirPath.isEmpty())
        return;

    if (m_app && m_app->dataController()) {
        m_app->dataController()->importExperiment(dirPath, m_currentVersion);
    }
}

void LayerExplorerWidget::handleRemoveElement() {
    QList<SelectedItem> ids = getSelectedIds();
    for (auto item : ids) {
        // Делегируем удаление через сигналы, удалит ноду из ObjectRegistry
        emit removalObjectRequested(item.id); // TODO не удалит несколько выделенных слоев
    }
}

void LayerExplorerWidget::handleNodeSelected() {
    QList<SelectedItem> selectedIds = getSelectedIds();
    // Оповещаем мир о массовом изменении
    emit selectionChanged(selectedIds);
}

// ---------------------------------------------------------
// @SECTION: Редактор слоев - Видимость слоев
// ---------------------------------------------------------

void LayerExplorerWidget::handleHideAll() {
    if (!m_treeModel)
        return;
    setCheckStateRecursiveInternal(QModelIndex(), Qt::Unchecked);
}

void LayerExplorerWidget::handleShowAll() {
    if (!m_treeModel)
        return;
    setCheckStateRecursiveInternal(QModelIndex(), Qt::Checked);
}

void LayerExplorerWidget::handleHideSelected() {
    if (!m_treeModel || !ui || !ui->treeView_Layers)
        return;
    auto* proxyModel = qobject_cast<QSortFilterProxyModel*>(ui->treeView_Layers->model());
    if (!proxyModel)
        return;
    QModelIndexList selectedProxyIndexes = ui->treeView_Layers->selectionModel()->selectedIndexes();
    for (const QModelIndex& proxyIndex : selectedProxyIndexes) {
        // Фильтруем по первой колонке, чтобы не обрабатывать одну строку несколько раз
        if (proxyIndex.column() != 0)
            continue;

        // Переводим прокси-индекс в индекс нашей m_treeModel
        QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);

        // Гасим выбранный элемент и всё, что находится внутри него
        setCheckStateRecursiveInternal(sourceIndex, Qt::Unchecked);
    }
}

void LayerExplorerWidget::handleShowSelected() {
    if (!m_treeModel || !ui || !ui->treeView_Layers)
        return;
    auto* proxyModel = qobject_cast<QSortFilterProxyModel*>(ui->treeView_Layers->model());
    if (!proxyModel)
        return;
    QModelIndexList selectedProxyIndexes = ui->treeView_Layers->selectionModel()->selectedIndexes();
    for (const QModelIndex& proxyIndex : selectedProxyIndexes) {
        // Фильтруем по первой колонке, чтобы не обрабатывать одну строку несколько раз
        if (proxyIndex.column() != 0)
            continue;

        // Переводим прокси-индекс в индекс нашей m_treeModel
        QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);

        // Гасим выбранный элемент и всё, что находится внутри него
        setCheckStateRecursiveInternal(sourceIndex, Qt::Checked);
    }
} // namespace QSpace::UI
void LayerExplorerWidget::handleHideUnselected() {
    if (!m_treeModel || !ui || !ui->treeView_Layers)
        return;

    auto* proxyModel = qobject_cast<QSortFilterProxyModel*>(ui->treeView_Layers->model());
    if (!proxyModel)
        return;

    // 1. Собираем ВСЕ выделенные прокси-индексы
    QModelIndexList selectedProxyIndexes = ui->treeView_Layers->selectionModel()->selectedIndexes();

    // 2. Переводим их в индексы m_treeModel и сохраняем в хэш-сет для быстрого поиска
    QSet<QModelIndex> selectedSourceIndexes;
    for (const QModelIndex& proxyIndex : selectedProxyIndexes) {
        if (proxyIndex.column() == 0) { // Нас интересует только первая колонка с чекбоксами
            selectedSourceIndexes.insert(proxyModel->mapToSource(proxyIndex));
        }
    }

    // 3. Запускаем рекурсивный поиск от корня дерева.
    // Всё, что не попало в selectedSourceIndexes, будет выключено.
    setCheckStateUnselectedRecursiveInternal(QModelIndex(), selectedSourceIndexes, Qt::Unchecked);
}

// ---------------------------------------------------------
// @SECTION: Редактор слоев - Фильтры слоев
// ---------------------------------------------------------
void LayerExplorerWidget::handleShowLoadedLayers(const bool isChecked) {
    Q_UNUSED(this);
    // Заглушка: Здесь можно реализовать проверку статуса ноды (загружена ли она в LRU кэш)
    // и скрывать/отображать соответствующие элементы QtreeView_Layers.
}
void LayerExplorerWidget::handleShowUnloadedLayers(const bool isChecked) {
    Q_UNUSED(this);

    // Заглушка: Аналогично для выгруженных слоев
}
void LayerExplorerWidget::handleFilterEquationToggled(const bool isChecked) {
    Q_UNUSED(this);

    // Активация фильтрации слоев на основе пользовательского уравнения/выражения
}
void LayerExplorerWidget::handleFilterEquationLineChange(const QString& line) {
    Q_UNUSED(this);
    // Обработка изменения выражения для фильтрации
}
// ---------------------------------------------------------
// @SECTION: Управление структурой
// ---------------------------------------------------------
void LayerExplorerWidget::handleExpandAll() {
    ui->treeView_Layers->expandAll();
}

void LayerExplorerWidget::handleCollapseAll() {
    ui->treeView_Layers->collapseAll();
}

void LayerExplorerWidget::handleCollapseAllFiles() {
    ui->treeViewFiles->collapseAll();
}

// ---------------------------------------------------------
// @SECTION: Проводник файлов
// ---------------------------------------------------------
void LayerExplorerWidget::handleRootPathChange(const QString& line) {
    QDir dir(line);
    if (dir.exists()) {
        m_root_path = line;
        if (auto* model = qobject_cast<QFileSystemModel*>(ui->treeViewFiles->model())) {
            ui->treeViewFiles->setRootIndex(model->index(m_root_path));
        }
    }
}
void LayerExplorerWidget::handleFindFile(const QString& line) {
    if (auto* model = qobject_cast<QFileSystemModel*>(ui->treeViewFiles->model())) {
        QStringList filters;
        if (line.isEmpty()) {
            // Возвращаем исходные фильтры
            filters << "*.bin" << "*.hdf5" << "*.csv" << "*.dat";
        } else {
            // Добавляем wildcard для поиска по подстроке
            filters << QString("*%1*").arg(line);
        }
        model->setNameFilters(filters);
    }
}
// ---------------------------------------------------------
// @SECTION: Дополнительные методы
// ---------------------------------------------------------

void LayerExplorerWidget::setCheckStateRecursiveInternal(const QModelIndex& parentIndex,
                                                         Qt::CheckState     state) {
    if (!m_treeModel)
        return;
    int rows = m_treeModel->rowCount(parentIndex);
    for (int i = 0; i < rows; ++i) {
        QModelIndex childIndex = m_treeModel->index(i, 0, parentIndex);
        m_treeModel->setData(childIndex, state, Qt::CheckStateRole);
        // Рекурсивно для всех детей
        if (m_treeModel->hasChildren(childIndex)) {
            setCheckStateRecursiveInternal(childIndex, state);
        }
    }
}
void LayerExplorerWidget::setCheckStateUnselectedRecursiveInternal(
    const QModelIndex&       parentIndex,
    const QSet<QModelIndex>& selectedSourceIndexes,
    Qt::CheckState           state) {
    if (!m_treeModel)
        return;

    int rows = m_treeModel->rowCount(parentIndex);
    for (int i = 0; i < rows; ++i) {
        QModelIndex currentIndex = m_treeModel->index(i, 0, parentIndex);

        // Если этого элемента НЕТ в списке выделенных — меняем его состояние
        if (!selectedSourceIndexes.contains(currentIndex)) {
            m_treeModel->setData(currentIndex, state, Qt::CheckStateRole);
        }

        // Идем глубже по дереву (даже если родитель выделен, его дети могут быть не выделены)
        if (m_treeModel->hasChildren(currentIndex)) {
            setCheckStateUnselectedRecursiveInternal(currentIndex, selectedSourceIndexes, state);
        }
    }
}

void LayerExplorerWidget::handleShowCustomContextMenuForFile(const QPoint& pos) {
    // получаем индекс из модели файлов по позиции курсора
    QModelIndex index = ui->treeViewFiles->indexAt(pos);

    if (!index.isValid())
        return;

    // проверяем, что выбранный элемент это файл а не папка
    auto* model = qobject_cast<QFileSystemModel*>(ui->treeViewFiles->model());

    if (!model)
        return;

    if (model->isDir(index))
        return;

    QMenu    contextMenu(this);
    QAction* actionImport = contextMenu.addAction(tr("Загрузить файл"));
    connect(actionImport,
            &QAction::triggered,
            this,
            &LayerExplorerWidget::handleImportFileRequestFromFileExplorer);
    contextMenu.exec(ui->treeViewFiles->viewport()->mapToGlobal(pos));
}
// --- Рекурсивный поиск по всему дереву ---
void LayerExplorerWidget::searchTreeRecursively(const QModelIndex&  parent,
                                                const QString&      text,
                                                QAbstractItemModel* model,
                                                QList<QModelIndex>& results) {
    int rows = model->rowCount(parent);
    for (int i = 0; i < rows; ++i) {
        // Проверяем только 0-ю колонку (NameColumn), где лежит текст
        QModelIndex idx  = model->index(i, 0, parent);
        QString     name = idx.data(Qt::DisplayRole).toString();

        if (name.contains(text, Qt::CaseInsensitive)) {
            results.append(idx);
        }

        // Если у узла есть дети — идем вглубь
        if (model->hasChildren(idx)) {
            searchTreeRecursively(idx, text, model, results);
        }
    }
}

// --- Построение контекстной строки (пути) для плоского списка ---
QString LayerExplorerWidget::buildItemContextString(const QModelIndex& index, QAbstractItemModel* model) {
    Q_UNUSED(model);
    QStringList path;
    QModelIndex curr = index;

    // Поднимаемся от найденного элемента вверх до корня
    while (curr.isValid()) {
        path.prepend(curr.data(Qt::DisplayRole).toString());
        curr = curr.parent();
    }

    // Результат будет выглядеть как "Эксперимент 1 -> Снапшот (t=0) -> Gas"
    return path.join(" -> ");
}

} // namespace QSpace::UI
#include "LayerExplorerWidget.moc"