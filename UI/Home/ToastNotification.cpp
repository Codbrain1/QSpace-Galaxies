#include "ToastNotification.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace QSpace::UI {

namespace {
constexpr int kMargin = 16;
constexpr int kDurationMs = 6000;
}

ToastNotification::ToastNotification(QWidget* parent) : QFrame(parent) {
    setObjectName("toastNotification");
    setFrameShape(QFrame::StyledPanel);
    setMaximumWidth(420);
    setStyleSheet(
        "QFrame#toastNotification { background-color: #4a2020; border: 1px solid #d85a5a; "
        "border-radius: 6px; }"
        "QLabel#toastTitle { color: #ffffff; font-weight: bold; }"
        "QLabel#toastMessage { color: #f4dddd; }"
        "QToolButton { color: #ffffff; border: none; padding: 2px; }"
        "QToolButton:hover { background-color: #713030; }");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 8, 10);
    layout->setSpacing(8);

    auto* textLayout = new QVBoxLayout;
    textLayout->setSpacing(4);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("toastTitle");
    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName("toastMessage");
    m_messageLabel->setWordWrap(true);
    textLayout->addWidget(m_titleLabel);
    textLayout->addWidget(m_messageLabel);

    m_closeButton = new QToolButton(this);
    m_closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    m_closeButton->setToolTip(tr("Закрыть уведомление"));
    layout->addLayout(textLayout, 1);
    layout->addWidget(m_closeButton, 0, Qt::AlignTop);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        hide();
        showNextError();
    });
    connect(m_closeButton, &QToolButton::clicked, this, [this]() {
        m_hideTimer->stop();
        hide();
        showNextError();
    });

    if (parent) {
        parent->installEventFilter(this);
    }
    hide();
}

void ToastNotification::showError(const QString& title, const QString& message) {
    m_pendingErrors.append({title, message});
    if (!isVisible()) {
        showNextError();
    }
}

bool ToastNotification::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
        positionInParent();
    }
    return QFrame::eventFilter(watched, event);
}

void ToastNotification::showNextError() {
    if (m_pendingErrors.isEmpty()) {
        return;
    }

    const ErrorMessage error = m_pendingErrors.takeFirst();
    m_titleLabel->setText(error.title);
    m_messageLabel->setText(error.message);
    adjustSize();
    positionInParent();
    show();
    raise();
    m_hideTimer->start(kDurationMs);
}

void ToastNotification::positionInParent() {
    if (!parentWidget()) {
        return;
    }

    const QRect area = parentWidget()->rect();
    move(area.right() - width() - kMargin, area.bottom() - height() - kMargin);
}

} // namespace QSpace::UI
