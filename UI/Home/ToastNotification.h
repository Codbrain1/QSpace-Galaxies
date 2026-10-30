#pragma once

#include <QFrame>
#include <QList>
#include <QString>

class QLabel;
class QToolButton;
class QTimer;

namespace QSpace::UI {

class ToastNotification final : public QFrame {
    Q_OBJECT

  public:
    explicit ToastNotification(QWidget* parent = nullptr);

    void showError(const QString& title, const QString& message);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    struct ErrorMessage {
        QString title;
        QString message;
    };

    void showNextError();
    void positionInParent();

    QLabel*      m_titleLabel;
    QLabel*      m_messageLabel;
    QToolButton* m_closeButton;
    QTimer*      m_hideTimer;
    QList<ErrorMessage> m_pendingErrors;
};

} // namespace QSpace::UI
