#include "SettingsEditorWidget.h"
#include "Visualize/Layers/LayerSettings.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMetaProperty>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <qcustomplot.h>

namespace QSpace::UI {
SettingsEditorWidget::SettingsEditorWidget(QWidget* parent)
    : QWidget(parent), m_layout(new QFormLayout(this)) {
    m_layout->setLabelAlignment(Qt::AlignLeft);
}

void SettingsEditorWidget::setSettings(QSpace::Visualize::Layers::LayerSettings* settings) {
    if (m_settings == settings)
        return;

    // Отключаем старый объект, если он был привязан
    if (m_settings)
        m_settings->disconnect(this);
    else {
        clearEditor();
    }

    m_settings = settings;
    clearEditor();

    if (!m_settings)
        return;

    const QMetaObject* mo = m_settings->metaObject();

    // Пропускаем свойства самого базового QObject и собираем всю цепочку наследования
    for (int i = QObject::staticMetaObject.propertyCount(); i < mo->propertyCount(); ++i) {
        QMetaProperty prop         = mo->property(i);
        QString       propertyName = QString::fromLatin1(prop.name());

        // Черный список: игнорируем свойства, которые PropertyInspector рендерит кастомно вверху панели
        if (propertyName == "colorMapId" || propertyName == "colorByField" || propertyName == "mode") {
            continue;
        }

        createRowForProperty(prop);
    }

    // Загружаем актуальные значения и выставляем доступность (setEnabled) полей
    updateUiValues();

    // Подписываемся на сигнал изменений изнутри движка/модели для автообновления UI
    connect(m_settings,
            &QSpace::Visualize::Layers::LayerSettings::changed,
            this,
            &SettingsEditorWidget::updateUiValues);
    connect(m_settings,
            &QSpace::Visualize::Layers::LayerSettings::calculatedRangeChanged,
            this,
            &SettingsEditorWidget::updateUiValues);
}

void SettingsEditorWidget::createRowForProperty(const QMetaProperty& prop) {
    QString         propertyName = QString::fromLatin1(prop.name());
    QVariant        value        = prop.read(m_settings);
    QMetaType::Type type         = static_cast<QMetaType::Type>(prop.metaType().id());
    QString         labelText    = m_settings->propertyDisplayName(propertyName);

    QWidget* editorWidget = nullptr;

    if (prop.isEnumType()) {
        auto*     combo    = new QComboBox(this);
        QMetaEnum metaEnum = prop.enumerator();

        for (int i = 0; i < metaEnum.keyCount(); ++i) {
            QString key = QString::fromLatin1(metaEnum.key(i));
            // Запрашиваем красивое имя у настроек
            QString displayName = m_settings->enumValueDisplayName(propertyName, key);
            combo->addItem(displayName, metaEnum.value(i));
        }

        combo->setCurrentIndex(combo->findData(value.toInt()));

        connect(combo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                [this, prop, combo](int index) {
                    int enumValue = combo->itemData(index).toInt();
                    if (m_settings)
                        prop.write(m_settings, enumValue);
                });
        editorWidget = combo;
    } else if (type == QMetaType::Bool) {
        auto* cb = new QCheckBox(this);
        cb->setChecked(value.toBool());
        connect(cb, &QCheckBox::toggled, this, [this, prop](bool checked) {
            if (m_settings)
                prop.write(m_settings, checked);
        });
        editorWidget = cb;
    } else if (type == QMetaType::Double || type == QMetaType::Float) {
        auto* sb = new QDoubleSpinBox(this);

        // Первично настраиваем ограничения из нашей структуры
        auto constraints = m_settings->propertyConstraints(propertyName);
        sb->setRange(constraints.min, constraints.max);
        sb->setDecimals(constraints.decimals);
        sb->setSingleStep(constraints.step);

        sb->setValue(value.toDouble());
        connect(sb, &QDoubleSpinBox::valueChanged, this, [this, prop](double val) {
            if (m_settings)
                prop.write(m_settings, val);
        });
        editorWidget = sb;
    } else if (type == QMetaType::Int) {
        auto* sb = new QSpinBox(this);
        sb->setRange(-1000000, 1000000);
        sb->setValue(value.toInt());
        connect(sb, &QSpinBox::valueChanged, this, [this, prop](int val) {
            if (m_settings)
                prop.write(m_settings, val);
        });
        editorWidget = sb;
    } else if (type == QMetaType::QString) {
        auto* le = new QLineEdit(this);
        le->setText(value.toString());
        connect(le, &QLineEdit::textChanged, this, [this, prop](const QString& text) {
            if (m_settings)
                prop.write(m_settings, text);
        });
        editorWidget = le;
    } else if (type == QMetaType::QColor) {
        auto* btn = new QPushButton(this);
        updateColorButton(btn, value.value<QColor>());
        connect(btn, &QPushButton::clicked, this, [this, btn, prop]() {
            QColor current = prop.read(m_settings).value<QColor>();
            QColor chosen  = QColorDialog::getColor(current, this, "Выберите цвет");
            if (chosen.isValid()) {
                if (m_settings)
                    prop.write(m_settings, chosen);
                updateColorButton(btn, chosen);
            }
        });
        editorWidget = btn;
    }

    if (editorWidget) {
        m_layout->addRow(labelText, editorWidget);
        m_editors[propertyName] = editorWidget;
    }
}

void SettingsEditorWidget::updateColorButton(QPushButton* btn, const QColor& color) {
    btn->setText(color.name(QColor::HexArgb));
    btn->setStyleSheet(QString("background-color: %1; color: %2; font-weight: bold;")
                           .arg(color.name())
                           .arg(color.lightness() > 128 ? "black" : "white"));
}

void SettingsEditorWidget::updateUiValues() {
    if (!m_settings)
        return;

    const QMetaObject* mo = m_settings->metaObject();

    for (auto it = m_editors.constBegin(); it != m_editors.constEnd(); ++it) {
        QString propName = it.key();
        int     idx      = mo->indexOfProperty(propName.toUtf8().constData());
        if (idx < 0)
            continue;

        QVariant value        = mo->property(idx).read(m_settings);
        QWidget* editorWidget = it.value();

        // Важно: блокируем сигналы конкретного виджета, чтобы изменение значения
        // из кода программно не триггерило обратно лямбду prop.write()
        QSignalBlocker blocker(editorWidget);

        if (auto* cb = qobject_cast<QCheckBox*>(editorWidget))
            cb->setChecked(value.toBool());
        else if (auto* sb = qobject_cast<QDoubleSpinBox*>(editorWidget)) {
            // КРИТИЧЕСКИЙ МОМЕНТ: Пересчитываем шаг и границы диапазона на лету!
            auto constraints = m_settings->propertyConstraints(propName);
            sb->setRange(constraints.min, constraints.max);
            sb->setDecimals(constraints.decimals);
            sb->setSingleStep(constraints.step); // Шаг адаптируется автоматически!

            sb->setValue(value.toDouble());
        } else if (auto* sb = qobject_cast<QSpinBox*>(editorWidget))
            sb->setValue(value.toInt());
        else if (auto* le = qobject_cast<QLineEdit*>(editorWidget))
            le->setText(value.toString());
        else if (auto* btn = qobject_cast<QPushButton*>(editorWidget))
            updateColorButton(btn, value.value<QColor>());

        // Запрашиваем у класса, активно ли свойство в текущий момент (например, заблокирован ли радиус)
        bool isEnabled = m_settings->isPropertyEnabled(propName);
        editorWidget->setEnabled(isEnabled);
    }
}

void SettingsEditorWidget::clearEditor() {
    m_editors.clear();
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget())
            delete item->widget();
        delete item;
    }
}
} // namespace QSpace::UI