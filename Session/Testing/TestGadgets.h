
#pragma once
#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVariant>
#include <qtmetamacros.h>

namespace TestTypes {
Q_NAMESPACE

// Простой enum с метаинформацией Qt
enum class Status { Unknown = 0, Active = 1, Disabled = 2 };
Q_ENUM_NS(Status) // Важно для работы QMetaEnum

// Простой гаджет
struct SimpleGadget {
    Q_GADGET
    Q_PROPERTY(int id READ id WRITE setId)
    Q_PROPERTY(QString name READ name WRITE setName)

  public:
    int id() const {
        return m_id;
    }

    void setId(int id) {
        m_id = id;
    }

    QString name() const {
        return m_name;
    }

    void setName(const QString& name) {
        m_name = name;
    }

    bool operator==(const SimpleGadget& other) const {
        return m_id == other.m_id && m_name == other.m_name;
    }

  private:
    int     m_id{0};
    QString m_name;
};

// Комплексный гаджет (вложенные типы, enum, списки)
struct ComplexGadget {
    Q_GADGET
    Q_PROPERTY(Status status READ status WRITE setStatus)
    Q_PROPERTY(SimpleGadget nested READ nested WRITE setNested)
    Q_PROPERTY(QList<int> numbers READ numbers WRITE setNumbers)
    Q_PROPERTY(
        QList<TestTypes::SimpleGadget> simpleGadgets READ simpleGadgets WRITE setSimpleGadgets)

  public:
    Status status() const {
        return m_status;
    }

    void setStatus(Status s) {
        m_status = s;
    }

    SimpleGadget nested() const {
        return m_nested;
    }

    void setNested(const SimpleGadget& n) {
        m_nested = n;
    }

    QList<int> numbers() const {
        return m_numbers;
    }

    void setNumbers(const QList<int>& nums) {
        m_numbers = nums;
    }

    QList<SimpleGadget> simpleGadgets() const {
        return m_simplegadgets;
    }

    void setSimpleGadgets(const QList<SimpleGadget>& sg) {
        m_simplegadgets = sg;
    };

    bool operator==(const ComplexGadget& other) const {
        return m_status == other.m_status && m_nested == other.nested() &&
               m_numbers == other.m_numbers;
    }

  private:
    Status              m_status{Status::Unknown};
    SimpleGadget        m_nested;
    QList<int>          m_numbers;
    QList<SimpleGadget> m_simplegadgets;
};

// Гаджет с последовательной и ассоциативной коллекциями.
struct QtValueGadget {
    Q_GADGET
    Q_PROPERTY(QUuid uuid READ uuid WRITE setUuid)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp)
    Q_PROPERTY(QHash<QString, int> values READ values WRITE setValues)
    Q_PROPERTY(QList<QUuid> ids READ ids WRITE setIds)

  public:
    QUuid uuid() const { return m_uuid; }
    void setUuid(const QUuid& value) { m_uuid = value; }
    QColor color() const { return m_color; }
    void setColor(const QColor& value) { m_color = value; }
    QDateTime timestamp() const { return m_timestamp; }
    void setTimestamp(const QDateTime& value) { m_timestamp = value; }
    QHash<QString, int> values() const { return m_values; }
    void setValues(const QHash<QString, int>& value) { m_values = value; }
    QList<QUuid> ids() const { return m_ids; }
    void setIds(const QList<QUuid>& value) { m_ids = value; }

  private:
    QUuid m_uuid;
    QColor m_color;
    QDateTime m_timestamp;
    QHash<QString, int> m_values;
    QList<QUuid> m_ids;
};

struct CollectionGadget {
    Q_GADGET
    Q_PROPERTY(QList<SimpleGadget> items READ items WRITE setItems)
    Q_PROPERTY(QMap<QString, SimpleGadget> itemsByName READ itemsByName WRITE setItemsByName)
    Q_PROPERTY(QMap<QString, int> counters READ counters WRITE setCounters)

  public:
    QList<SimpleGadget> items() const {
        return m_items;
    }

    void setItems(const QList<SimpleGadget>& items) {
        m_items = items;
    }

    QMap<QString, SimpleGadget> itemsByName() const {
        return m_itemsByName;
    }

    void setItemsByName(const QMap<QString, SimpleGadget>& itemsByName) {
        m_itemsByName = itemsByName;
    }

    QMap<QString, int> counters() const {
        return m_counters;
    }

    void setCounters(const QMap<QString, int>& counters) {
        m_counters = counters;
    }

  private:
    QList<SimpleGadget>        m_items;
    QMap<QString, SimpleGadget> m_itemsByName;
    QMap<QString, int>          m_counters;
};

} // namespace TestTypes

// Регистрируем гаджеты в Meta-object системе Qt
Q_DECLARE_METATYPE(TestTypes::SimpleGadget)
Q_DECLARE_METATYPE(TestTypes::ComplexGadget)
Q_DECLARE_METATYPE(TestTypes::CollectionGadget)
Q_DECLARE_METATYPE(TestTypes::QtValueGadget)
using IntHash = QHash<QString, int>;
Q_DECLARE_METATYPE(IntHash)
Q_DECLARE_METATYPE(QList<TestTypes::SimpleGadget>)
using SimpleGadgetMap = QMap<QString, TestTypes::SimpleGadget>;
using IntMap          = QMap<QString, int>;
Q_DECLARE_METATYPE(SimpleGadgetMap)
Q_DECLARE_METATYPE(IntMap)