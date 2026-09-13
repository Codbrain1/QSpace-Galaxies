
#pragma once
#include <QList>
#include <QObject>
#include <QString>
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

} // namespace TestTypes

// Регистрируем гаджеты в Meta-object системе Qt
Q_DECLARE_METATYPE(TestTypes::SimpleGadget)
Q_DECLARE_METATYPE(TestTypes::ComplexGadget)