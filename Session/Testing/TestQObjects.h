#pragma once
#include <QObject>
#include <qtmetamacros.h>

namespace TestTypes {
Q_NAMESPACE

// Простой enum с метаинформацией Qt
enum class Status { Unknown = 0, Active = 1, Disabled = 2 };
Q_ENUM_NS(Status) // Важно для работы QMetaEnum

// Создаем тестовый QObject класс прямо в файле тестов
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