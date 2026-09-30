#pragma once
#include "Common/Interfaces/IRenderLayer.h"
#include "Common/Structures/ObjectRegistryStructures.h"
#include "Visualize/Views/AbstractView.h"
#include "Visualize/Views/View3D/AbstractView3D.h"
#include <QObject>
#include <QString>
#include <QUuid>
#include <quuid.h>
#include "LayerSettings.h"
#include <memory>

namespace QSpace::Visualize::Layers {

class Layer : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUuid layerId READ layerId CONSTANT)
    Q_PROPERTY(QUuid dataNodeId READ dataNodeId CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool isSyncedWithMaster READ IsSyncedWithMaster WRITE setIsSyncedWithMaster NOTIFY
                   syncStatusChanged)

  public:
    Layer(std::shared_ptr<Core::DataNode>                 node,
          std::shared_ptr<Visualize::Views::AbstractView> targetView,
          QObject*                                        parent = nullptr)
        : QObject(parent),
          m_layerId(QUuid::createUuid()),
          m_dataNodeId(node ? node->id : QUuid()),
          m_name(node ? node->label : QString()),
          m_dataNode(node),
          m_view(targetView),
          m_settings(nullptr) {
    }

    Layer(const QString&                                  name,
          std::shared_ptr<Visualize::Views::AbstractView> targetView,
          QObject*                                        parent = nullptr)
        : QObject(parent),
          m_layerId(QUuid::createUuid()),
          m_name(name),
          m_view(targetView),
          m_settings(nullptr) {
    }

    ~Layer() {
        if (auto view3D = std::dynamic_pointer_cast<QSpace::Visualize::Views::AbstractView3D>(
                m_view.lock())) {
            view3D->detachRenderLayer(m_layerId);
        }
    }

    std::weak_ptr<Views::AbstractView> getView() {
        return m_view;
    }

    // Геттеры для Q_PROPERTY
    QUuid layerId() const {
        return m_layerId;
    }

    QUuid dataNodeId() const {
        return m_dataNodeId;
    }

    QString name() const {
        return m_name;
    }

    bool IsSyncedWithMaster() const {
        return m_isSyncedWithMaster;
    }

    // Сеттеры для Q_PROPERTY
    void setLayerId(const QUuid& id) {
        if (m_layerId != id) {
            m_layerId = id;
        }
    }

    void setName(const QString& name) {
        if (m_name != name) {
            m_name = name;
            emit nameChanged(m_name);
        }
    }

    // TODO метод не используется при добавлении многооконности может понадобится
    void setIsSyncedWithMaster(bool synced) {
        if (m_isSyncedWithMaster != synced) {
            m_isSyncedWithMaster = synced;
            emit syncStatusChanged(m_isSyncedWithMaster);
        }
    }

    // Доступ к сырым объектам внутри слоя
    std::shared_ptr<LayerSettings> getSettings() const {
        return m_settings;
    }

    std::shared_ptr<Visualize::IRenderLayer> getEngine() {
        return m_renderEngine;
    }

    void assignEngine(std::shared_ptr<Visualize::IRenderLayer> engine) {
        m_renderEngine = engine;
        m_settings     = engine->getSettings();
        if (m_settings) {
            connect(m_settings.get(), &LayerSettings::changed, this, &Layer::update);
        }
        if (m_renderEngine) {
            if (auto view3D = qobject_cast<Views::AbstractView3D*>(m_view.lock().get())) {
                view3D->attachRenderLayer(m_layerId, engine);
            }
        }
    }

    void setVisible(bool visible) {
        if (m_settings)
            m_settings->setVisible(visible);
        if (m_renderEngine)
            m_renderEngine->setVisible(visible);
        update();
    }

    void setData(std::shared_ptr<Core::DataNode> node) {
        m_dataNode   = node;
        m_dataNodeId = node->id;
        m_renderEngine->setData(node);
        m_renderEngine->update();
    }

    void update() {
        auto v = m_view.lock();
        auto d = m_dataNode.lock();
        if (v && d && m_renderEngine) {
            m_renderEngine->update();
            v->render(); // TODO из за этого может тормозить рендеринг
            emit updateRequired();
        }
    }

  signals:
    void nameChanged(const QString& newName);
    void syncStatusChanged(bool isSynced);

    // Сигнал без аргументов, так как окно извлекается напрямую через m_view.lock()
    void updateRequired();

  private:
    QUuid                                         m_layerId;
    QUuid                                         m_dataNodeId;
    QString                                       m_name;
    std::weak_ptr<Core::DataNode>                 m_dataNode;
    std::weak_ptr<Visualize::Views::AbstractView> m_view;
    std::shared_ptr<LayerSettings>                m_settings;
    std::shared_ptr<Visualize::IRenderLayer>      m_renderEngine;
    bool                                          m_isSyncedWithMaster = true;
};

} // namespace QSpace::Visualize::Layers