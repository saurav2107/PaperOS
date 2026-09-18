#include "app/AppManager.h"
#include "app/AppContext.h"
#include "services/Services.h"

AppManager::AppManager(AppContext& context) : context_(context) {}

bool AppManager::registerApp(IApp& app) {
  if (appCount_ >= sizeof(apps_) / sizeof(apps_[0])) return false;
  apps_[appCount_++] = &app;
  return true;
}

IApp* AppManager::find(AppId id) const {
  for (uint8_t i = 0; i < appCount_; ++i) {
    if (apps_[i]->id() == id) return apps_[i];
  }
  return nullptr;
}

bool AppManager::activate(AppId id) {
  IApp* next = find(id);
  if (!next) return false;
  if (active_ && active_->id() == id) return true;

  if (active_) {
    context_.tasks.stopOwnedBy(static_cast<uint8_t>(active_->id()));
    active_->onStop(context_);
    context_.display.resetToLauncherState();
  }
  active_ = next;
  return active_->onStart(context_);
}

void AppManager::tick() {
  if (active_) active_->onTick(context_, millis());
}

AppId AppManager::activeId() const {
  return active_ ? active_->id() : AppId::Launcher;
}
