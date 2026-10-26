#pragma once
#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QUuid>
#include <qtmetamacros.h>

namespace TestTypes {
Q_NAMESPACE

// Простой enum с метаинформацией Qt
enum class Status { Unknown = 0, Active = 1, Disabled = 2 };
Q_ENUM_NS(Status) // Важно для работы QMetaEnum

// Создаем тестовый QObject класс прямо в файле тестов
class TestValueObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUuid uuid READ uuid WRITE setUuid)
    Q_PROPERTY(TestTypes::Status status READ status WRITE setStatus)
    Q_PROPERTY(QList<int> numbers READ numbers WRITE setNumbers)
    Q_PROPERTY(QHash<QString, int> values READ values WRITE setValues)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp)

  public:
    Q_INVOKABLE explicit TestValueObject(QObject* parent = nullptr) : QObject(parent) {}
    QUuid uuid() const { return m_uuid; }
    void setUuid(const QUuid& value) { m_uuid = value; }
    TestTypes::Status status() const { return m_status; }
    void setStatus(TestTypes::Status value) { m_status = value; }
    QList<int> numbers() const { return m_numbers; }
    void setNumbers(const QList<int>& value) { m_numbers = value; }
    QHash<QString, int> values() const { return m_values; }
    void setValues(const QHash<QString, int>& value) { m_values = value; }
    QColor color() const { return m_color; }
    void setColor(const QColor& value) { m_color = value; }
    QDateTime timestamp() const { return m_timestamp; }
    void setTimestamp(const QDateTime& value) { m_timestamp = value; }

  private:
    QUuid m_uuid;
    TestTypes::Status m_status = TestTypes::Status::Unknown;
    QList<int> m_numbers;
    QHash<QString, int> m_values;
    QColor m_color;
    QDateTime m_timestamp;
};

class TestObject : public QObject {
    Q_OBJECT

    // Стандартные свойства для чтения и записи
    Q_PROPERTY(int id READ id WRITE setId)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive)

    // Свойство только для чтения (чтобы проверить readOnlyKeys в DeserializationResult)
    Q_PROPERTY(QString readOnlyField READ readOnlyField)

  public:
    Q_INVOKABLE explicit TestObject(QObject* parent = nullptr) : QObject(parent) {
    }

    int id() const {
        return m_id;
    }

    void setId(int id) {
        m_id = id;
    }

    QString title() const {
        return m_title;
    }

    void setTitle(const QString& title) {
        m_title = title;
    }

    bool isActive() const {
        return m_isActive;
    }

    void setIsActive(bool active) {
        m_isActive = active;
    }

    QString readOnlyField() const {
        return "ConstantData";
    }

    // Удобный оператор сравнения для тестов (сравниваем только изменяемые поля)
    bool operator==(const TestObject& other) const {
        return m_id == other.m_id && m_title == other.m_title && m_isActive == other.m_isActive &&
               objectName() == other.objectName();
    }

  private:
    int     m_id = 0;
    QString m_title;
    bool    m_isActive = false;
};

// Класс-наследник
class TestObjectDerived : public TestObject {
    Q_OBJECT

    // Свойство, существующее только у наследника
    Q_PROPERTY(double extraValue READ extraValue WRITE setExtraValue)
    Q_PROPERTY(TestTypes::Status status READ status WRITE setStatus)

  public:
    Q_INVOKABLE explicit TestObjectDerived(QObject* parent = nullptr) : TestObject(parent) {
    }

    double extraValue() const {
        return m_extraValue;
    }

    void setExtraValue(double val) {
        m_extraValue = val;
    }

    Status status() const {
        return m_status;
    }

    void setStatus(Status s) {
        m_status = s;
    }

  private:
    double m_extraValue = 0.0;
    Status m_status     = Status::Active;
};
} // namespace TestTypes

Q_DECLARE_METATYPE(TestTypes::Status)