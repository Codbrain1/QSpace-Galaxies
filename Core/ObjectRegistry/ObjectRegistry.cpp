#include "ObjectRegistry.h"
#include "Common/Logger/Logger.h"
#include "Common/Structures/ObjectRegistryStructures.h"
#include <qcontainerfwd.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <quuid.h>
#include "Physics/Math/MetaDataCalculating.h"
#include <memory>

namespace QSpace::Core {
ObjectRegistry::ObjectRegistry(QObject* parent) : QObject(parent) {
}

void ObjectRegistry::registerNode(std::shared_ptr<DataNode> node) {
    if (!node)
        return;
    if (m_nodes.contains(node->id)) {
        qCWarning(LogCore) << "Registry: Attempt to register duplicate node ID:" << node->id;
        return;
    }
    m_nodes.insert(node->id, node);
    if (node->data != nullptr) {
        node->stats = QSpace::Physics::Math::calculateMetaData(node->data, node->stats.timestamp);
        touchNodeInMemory(node->id);
    }
    emit nodeAdded(node, QUuid());
    qCInfo(LogCore) << "Registry: Node registered " << node->label << " " << node->id;
}

void ObjectRegistry::registerNodeToSnapshot(std::shared_ptr<DataNode> node,
                                            const QUuid&              parentSnapshotId) {
    if (!node || m_nodes.contains(node->id))
        return;

    m_nodes.insert(node->id, node);
    if (node->data != nullptr) {
        node->stats = QSpace::Physics::Math::calculateMetaData(node->data, node->stats.timestamp);
        touchNodeInMemory(node->id);
    }

    // Сигнал с родителем
    emit nodeAdded(node, parentSnapshotId);
    qCInfo(LogCore) << "Registry: Node registered " << node->label;
}

void ObjectRegistry::registerNodeToExperiment(std::shared_ptr<DataNode> node,
                                              const QUuid&              experimentId) {
    if (!node)
        return;

    auto targetExperiment = m_experiments.value(experimentId);
    if (!targetExperiment) {
        qCWarning(LogCore) << "Registry: Experiment ID not found for grouping:" << experimentId;
        return;
    }

    // 1. Ищем Снапшот внутри найденного эксперимента
    double                    ts             = node->stats.timestamp;
    const double              eps            = 1e-5;
    std::shared_ptr<Snapshot> targetSnapshot = nullptr;
    bool                      isNewSnapshot  = false;

    for (const auto& snap : targetExperiment->snapshots) {
        if (std::abs(snap->timestamp - ts) < eps) {
            targetSnapshot = snap;
            break;
        }
    }

    // 2. Создаем новый снапшот, если не нашли (но пока не регистрируем, чтобы не спамить сигналами)
    if (!targetSnapshot) {
        QString snapName = QString("Snapshot (t = %1)").arg(ts, 0, 'f', 5);
        targetSnapshot   = std::make_shared<Snapshot>(snapName, ts);

        targetExperiment->addSnapshot(targetSnapshot); // Привязываем к эксперименту
        isNewSnapshot = true;
    }

    // 3. СВЯЗЫВАЕМ данные: добавляем ноду в компоненты снапшота ДО любых сигналов
    targetSnapshot->addComponent(node);

    // 4. ГЕНЕРИРУЕМ сигналы: теперь иерархия в памяти полностью консистентна
    if (isNewSnapshot) {
        // Регистрируем в m_snapshots (вызовет emit snapshotAdded)
        // Модель создаст ветку снапшота и сразу увидит внутри нее node, так как addComponent уже
        // вызван
        registerSnapshot(targetSnapshot, experimentId);
    }

    // 5. Регистрируем атомарную ноду (вызовет emit nodeAdded)
    // Модель поймает сигнал, проверит к какому снапшоту относится нода,
    // найдет ее внутри targetSnapshot->components и корректно создаст DataTreeItem
    registerNodeToSnapshot(node, targetSnapshot->id);
}

void ObjectRegistry::registerSnapshot(std::shared_ptr<Snapshot> snapshot) {
    if (!snapshot)
        return;
    if (m_snapshots.contains(snapshot->id)) {
        qCWarning(LogCore) << "Registry: Attempt to register duplicate snapshot ID:"
                           << snapshot->id;
        return;
    }
    m_snapshots.insert(snapshot->id, snapshot);
    for (auto& comp : snapshot->components) {
        registerNode(comp);
    }
    emit snapshotAdded(snapshot, QUuid());
    qCDebug(LogCore) << "Registry: Snapshot registered " << snapshot->name << " " << snapshot->id;
}

void ObjectRegistry::registerSnapshot(std::shared_ptr<Snapshot> snapshot,
                                      const QUuid&              experimentId) {
    if (!snapshot || m_snapshots.contains(snapshot->id))
        return;

    m_snapshots.insert(snapshot->id, snapshot);

    // Отправляем сигнал, который DataTreeModel легко перехватит
    emit snapshotAdded(snapshot, experimentId);
    qCDebug(LogCore) << "Registry: Snapshot registered " << snapshot->name;

    for (auto& comp : snapshot->components) {
        registerNodeToSnapshot(comp, snapshot->id); // Передаем ID снапшота как родителя!
    }
}

void ObjectRegistry::registerExperiment(std::shared_ptr<Experiment> experiment) {
    if (!experiment)
        return;
    if (m_experiments.contains(experiment->id)) {
        qCWarning(LogCore) << "Registry: Attempt to register duplicate experiment ID:"
                           << experiment->id;
        return;
    }
    m_experiments.insert(experiment->id, experiment);
    emit experimentAdded(experiment);
    for (auto& snap : experiment->snapshots) {
        registerSnapshot(snap, experiment->id);
    }
    qCDebug(LogCore) << "Registry: Experiment registered " << experiment->name << " "
                     << experiment->id;
}

std::shared_ptr<DataNode> ObjectRegistry::getNode(const QUuid& id) const {
    return m_nodes.value(id, nullptr);
}

std::shared_ptr<Snapshot> ObjectRegistry::getSnapshot(const QUuid& id) const {
    return m_snapshots.value(id, nullptr);
}

std::shared_ptr<Experiment> ObjectRegistry::getExperiment(const QUuid& id) const {
    return m_experiments.value(id, nullptr);
}

std::shared_ptr<DataNode> ObjectRegistry::getOrLoadNodeData(const QUuid& id) {
    auto node = m_nodes.value(id, nullptr);
    if (!node)
        return nullptr;

    if (node->data != nullptr) {
        touchNodeInMemory(id); // Двигаем в начало кэша, возможно вытесняя старые
        return node;
    }

    // Если данных в ОЗУ нет — читаем с диска
    qCInfo(LogCore) << "LRU Cache: Lazy loading heavy VTK data for" << node->label;
    // emit dataLoadRequested(id, node->path, node->scheme);
    //  CRITICAL: разобраться с подгрузкой данных
    return node;
}

void ObjectRegistry::removeObject(const QUuid& id) {
    // =========================================================================
    // СЦЕНАРИЙ 1: УДАЛЕНИЕ ЭКСПЕРИМЕНТА (Самый верхний уровень)
    // =========================================================================
    if (m_experiments.contains(id)) {
        auto experiment = m_experiments.take(id);
        qCInfo(LogCore) << "Registry: Removing entire experiment:" << experiment->name;

        for (const auto& snap : experiment->snapshots) {
            if (!snap)
                continue;

            // Удаляем ноды этого конкретного снапшота
            for (const auto& node : snap->components) {
                if (!node)
                    continue;

                // Стираем ноду из глобального реестра и LRU (прямые O(1) операции)
                m_nodes.remove(node->id);
                if (m_lruMap.contains(node->id)) {
                    m_lruList.erase(m_lruMap[node->id]);
                    m_lruMap.remove(node->id);
                }
                emit objectRemoved(node->id);
            }

            // Удаляем сам снапшот из реестра контейнеров
            m_snapshots.remove(snap->id);
            emit objectRemoved(snap->id);
        }

        emit objectRemoved(id); // Оповещаем UI об удалении эксперимента
        return;
    }

    // =========================================================================
    // СЦЕНАРИЙ 2: УДАЛЕНИЕ ОДИНОЧНОГО КОНТЕЙНЕРА (Снапшота)
    // =========================================================================
    if (m_snapshots.contains(id)) {
        auto snapshot = m_snapshots.take(id);
        qCInfo(LogCore) << "Registry: Removing snapshot:" << snapshot->name;

        // 1. Сначала убираем ссылку на этот снапшот из его родительского эксперимента
        for (auto& exp : m_experiments) {
            auto It = std::find(exp->snapshots.begin(), exp->snapshots.end(), snapshot);
            if (It != exp->snapshots.end()) {
                exp->snapshots.erase(It);
                break; // Снапшот принадлежит только одному эксперименту
            }
        }

        // 2. Точечно удаляем только те ноды, которые принадлежали этому снапшоту
        for (const auto& node : snapshot->components) {
            if (!node)
                continue;

            m_nodes.remove(node->id);
            if (m_lruMap.contains(node->id)) {
                m_lruList.erase(m_lruMap[node->id]);
                m_lruMap.remove(node->id);
            }
            emit objectRemoved(node->id);
        }

        emit objectRemoved(id);
        return;
    }

    // =========================================================================
    // СЦЕНАРИЙ 3: УДАЛЕНИЕ ОДИНОЧНОЙ НОДЫ (Например, пользователь удалил слой газа)
    // =========================================================================
    if (m_nodes.contains(id)) {
        // Тут полный перебор контейнеров оправдан, т.к. мы не знаем, где именно лежит нода.
        // Но это работает быстро, потому что вызывается редко и НЕ рекурсивно!
        for (auto& snapshot : m_snapshots) {
            auto& comps = snapshot->components;
            auto  it    = std::remove_if(
                comps.begin(),
                comps.end(),
                [&id](const std::shared_ptr<DataNode>& n) { return n && n->id == id; });
            if (it != comps.end()) {
                comps.erase(it, comps.end());
                break; // Нода уникальна и лежит в одном конкретном временном шаге
            }
        }

        // Чистим LRU
        if (m_lruMap.contains(id)) {
            m_lruList.erase(m_lruMap[id]);
            m_lruMap.remove(id);
        }

        m_nodes.remove(id);
        emit objectRemoved(id);
        qCInfo(LogCore) << "Registry: Single node removed:" << id;
    }
}

std::shared_ptr<Snapshot> ObjectRegistry::findSnapshotByName(const QString& name) const {
    for (auto snapshot : m_snapshots) {
        if (snapshot->name == name) {
            return snapshot;
        }
    }
    return nullptr;
}

std::shared_ptr<Experiment> ObjectRegistry::findExperimentByName(const QString& name) const {
    for (auto experiment : m_experiments) {
        if (experiment->name == name) {
            return experiment;
        }
    }
    return nullptr;
}

void ObjectRegistry::updateNodeData(const QUuid&                id,
                                    vtkSmartPointer<vtkDataSet> dataSet,
                                    double                      timestamp) {
    auto node = m_nodes.value(id);
    if (!node)
        return;

    // Защита от лишних сигналов: если указатели совпадают, ничего не делаем
    if (node->data == dataSet)
        return;

    // Сохраняем предыдущее состояние для логики оповещений
    bool wasInMemory     = (node->data != nullptr);
    bool turningIntoNull = (dataSet == nullptr);

    // Применяем новые данные
    node->data = dataSet;

    if (!turningIntoNull) {
        // Данные появились/обновились в ОЗУ -> регистрируем в LRU
        node->stats = QSpace::Physics::Math::calculateMetaData(node->data, timestamp);
        touchNodeInMemory(id);

        // На случай, если лимит кэша жестко изменился в настройках,
        // гарантируем, что размер LRU не превышает емкость (заменяем if на while внутри touch или
        // здесь)
        while (m_lruList.size() > m_cacheCapacity) {
            QUuid oldestId   = m_lruList.back();
            auto  oldestNode = m_nodes.value(oldestId);
            if (oldestNode && oldestId != id) { // Не выталкиваем только что добавленную ноду
                oldestNode->data = nullptr;
                m_lruMap.remove(oldestId);
                m_lruList.pop_back();
                emit nodeDataUpdated(oldestId); // Оповещаем, что старая нода выгружена
                qCInfo(LogCore) << "LRU Cache: Evicted" << oldestNode->label << "due to overflow.";
            } else {
                break;
            }
        }
    } else {
        // Данные принудительно выгрузили (dataSet == nullptr)
        if (m_lruMap.contains(id)) {
            m_lruList.erase(m_lruMap[id]);
            m_lruMap.remove(id);
        }
    }

    // Генерируем сигнал только если статус "В ОЗУ / На Диске" реально изменился
    if (wasInMemory != (!turningIntoNull)) {
        emit nodeDataUpdated(id);
    }
}

void ObjectRegistry::touchNodeInMemory(const QUuid& id) {
    if (m_lruMap.contains(id)) {
        m_lruList.erase(m_lruMap[id]);
    }
    m_lruList.push_front(id);
    m_lruMap[id] = m_lruList.begin();

    if (m_lruList.size() > m_cacheCapacity) {
        QUuid oldestId   = m_lruList.back();
        auto  oldestNode = m_nodes.value(oldestId);

        if (oldestNode && oldestNode->data != nullptr) {
            oldestNode->data = nullptr; // Освобождаем память VTK
            qCInfo(LogCore) << "LRU Cache: Evicted data for node" << oldestNode->label;

            // Оповещаем UI, что статус памяти изменился
            emit nodeDataUpdated(oldestId);
        }

        m_lruMap.remove(oldestId);
        m_lruList.pop_back();
    }
}

void ObjectRegistry::clear() {
    m_nodes.clear();
    m_snapshots.clear();
    m_experiments.clear();
    m_lruList.clear();
    m_lruMap.clear();
    emit cleared();
}
} // namespace QSpace::Core