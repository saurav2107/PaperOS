#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"
#include "ui/Chrome.h"

// A deliberately bounded SD file browser.  It lists directories lazily and
// stores only metadata for the current folder, not the contents of files.
class FileBrowserApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Files; }
  const char* title() const override { return "Files"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override {}

 private:
  struct Entry { String path; bool directory{false}; size_t size{0}; };
  static constexpr uint8_t MaxEntries = 48;
  static constexpr uint8_t PageSize = 6;

  void scan(AppContext& context);
  void draw(AppContext& context);
  void drawRows();
  void drawCommands();
  void drawDeleteConfirmation();
  void selectOrOpen(AppContext& context, uint8_t index);
  void toggleSelection(AppContext& context);
  void createFolder(AppContext& context);
  void deleteSelected(AppContext& context);
  void confirmDelete(AppContext& context);
  void beginMove(AppContext& context);
  void moveHere(AppContext& context);
  void openDirectory(AppContext& context, const String& path);
  void goBack(AppContext& context);
  ui::ChromeOptions chromeOptions() const;
  String parentPath() const;
  String nameOf(const String& path) const;
  String joinPath(const String& folder, const String& name) const;

  Entry entries_[MaxEntries];
  uint8_t entryCount_{0};
  uint8_t page_{0};
  int8_t selected_{-1};
  bool selectionMode_{false};
  bool deleteConfirmationVisible_{false};
  bool moveMode_{false};
  String moveSource_;
  String currentPath_{"/"};
  String status_;
  NavigationCallback navigator_{nullptr};
};
