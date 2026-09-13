#pragma once
#include "Common/Enums/ViewEnums.h"
#include <QObject>
#include <QUuid>
#include <QtWidgets/qwidget.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <quuid.h>
#include "QVariantMap"

namespace QSpace::Visualize::Views {

class AbstractView : public QObject {
    Q_OBJECT
  public:
    explicit AbstractView(QObject* parent = nullptr)
        : QObject(parent), m_id(QUuid::createUuid()) {};

    virtual ~AbstractView() = default;

    virtual QWidget* getWidget() = 0; // встраивается в QDockWidget или QMainWindow

    // Общий контракт перерисовки
    virtual void render()                                         = 0;
    virtual void setBackgroundColor(double r, double g, double b) = 0;

    virtual void        setAxisVisible(bool visible)                           = 0;
    virtual void        setGridVisible(bool visible)                           = 0;
    virtual void        resetCamera()                                          = 0;
    virtual ViewType    viewType()                                             = 0;
    virtual QVariantMap getSettingsToVariantMap() const                        = 0;
    virtual bool        setSettingsFromVariantMap(const QVariantMap& settings) = 0;

    // --- Общая логика ---
    inline QUuid id() const {
        return m_id;
    };

    inline void setId(QUuid id) {
        m_id = id;
    }

    inline QString viewName() const {
        return m_viewName;
    }

    inline void setViewName(const QString& name) {
        m_viewName = name;
    }

  signals:
    void updateRequested();

  protected:
    QUuid   m_id;
    QString m_viewName;
};
} // namespace QSpace::Visualize::Views