#include <oclero/qlementine/utils/MenuUtils.hpp>

#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QPointer>

namespace oclero::qlementine {
class FlashActionHelper : public QObject {
public:
  FlashActionHelper(QAction* action, QMenu* menu, const std::function<void()>& onAnimationFinished)
    // Set parent to nullptr to manage lifecycle independently from the action
    : QObject(nullptr)
    , _menu(menu)
    , _action(action)
    , _onAnimationFinished(onAnimationFinished) {
    if (_menu && _action) {
      // Connect to the action's destroyed signal to prevent accessing it after deletion
      connect(_action, &QObject::destroyed, this, &FlashActionHelper::onActionDestroyed);
      _action->setProperty("qlementine_flashing", true);
      _menu->blockSignals(true);
      _timerId = startTimer(flashActionBlinkDuration);
    } else {
      // If we don't have valid action/menu, clean up immediately
      QTimer::singleShot(0, this, &QObject::deleteLater);
    }
  }

protected:
  void timerEvent(QTimerEvent*) override {
    if (_flashActionElapsedTime < flashActionDuration && _menu && _action) {
      _flashActionElapsedTime += flashActionBlinkDuration;
      const auto* currentActiveAction = _menu->activeAction();
      _menu->setActiveAction(currentActiveAction == nullptr ? _action : nullptr);
    } else {
      cleanup();
    }
  }

private slots:
  void onActionDestroyed() {
    // Action was destroyed, clear the pointer and clean up
    _action = nullptr;
    cleanup();
  }

private:
  void cleanup() {
    if (_timerId != -1) {
      killTimer(_timerId);
      _timerId = -1;
    }

    if (_menu) {
      if (_action) {
        _menu->setActiveAction(_action);
        _action->setProperty("qlementine_flashing", false);
      }
      _menu->blockSignals(false);
    }

    if (_onAnimationFinished) {
      _onAnimationFinished();
      // Clear the callback to prevent issues if cleanup is called multiple times
      _onAnimationFinished = nullptr;
    }

    // Schedule deletion on next event loop iteration
    deleteLater();
  }

  static constexpr int flashActionBlinkDuration{ 60 }; // ms
  static constexpr int flashActionDuration{ 2 * flashActionBlinkDuration }; // ms
  int _flashActionElapsedTime{ 0 }; // ms
  int _timerId{ -1 };
  QPointer<QMenu> _menu{ nullptr };
  QPointer<QAction> _action{ nullptr };
  std::function<void()> _onAnimationFinished{};
};

QMenu* getTopLevelMenu(QMenu* menu) {
  auto parent = menu;
  while (parent != nullptr) {
    auto parent_menu = qobject_cast<QMenu*>(parent->parentWidget());
    if (parent_menu != nullptr)
      parent = parent_menu;
    else
      break;
  }
  return parent;
}

void flashAction(QAction* action, QMenu* menu, const std::function<void()>& onAnimationFinished) {
  new FlashActionHelper(action, menu, onAnimationFinished);
}
} // namespace oclero::qlementine
