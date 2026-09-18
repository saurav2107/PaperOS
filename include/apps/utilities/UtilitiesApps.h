#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// Each utility has its own app identity and lifecycle. They share only a
// compact footer/header renderer through UtilityAppBase.
class UtilityAppBase : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  void onStop(AppContext&) override {}
 protected:
  void drawShell(AppContext& context, const char* title) const;
  bool handleHome(int x, int y) const;
  NavigationCallback navigator_{nullptr};
};

class CalculatorApp final : public UtilityAppBase {
 public:
  AppId id() const override { return AppId::Calculator; }
  const char* title() const override { return "Calculator"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 private:
  void draw(AppContext& context);
  void drawDisplay() const;
  void applyPending();
  String value_{"0"};
  String previousEntry_{};
  float accumulator_{0.0f};
  float memory_{0.0f};
  char pending_{0};
  bool startsNewValue_{true};
};

class ConverterApp final : public UtilityAppBase {
 public:
  AppId id() const override { return AppId::Converter; }
  const char* title() const override { return "Unit Converter"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 private:
  void draw(AppContext& context);
  String value_{"1"}; uint8_t unit_{0};
};

class AlarmApp final : public UtilityAppBase {
 public:
  AppId id() const override { return AppId::Alarm; }
  const char* title() const override { return "Alarm & Reminders"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 private:
  void draw(AppContext& context);
  uint8_t hour_{7}, minute_{0}; bool enabled_{false};
};

class ShoppingListApp final : public UtilityAppBase {
 public:
  AppId id() const override { return AppId::ShoppingList; }
  const char* title() const override { return "Shopping List"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 private:
  void load(AppContext& context); void save(AppContext& context); void draw(AppContext& context);
  String items_[6]; bool checked_[6]{}; uint8_t count_{0}; String status_{};
};

class BackupRestoreApp final : public UtilityAppBase {
 public:
  AppId id() const override { return AppId::BackupRestore; }
  const char* title() const override { return "Backup & Restore"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 private:
  void draw(AppContext& context); String status_{};
};

// A deliberately small, offline writing prototype.  Strokes—not screen
// bitmaps—are kept in fixed-size buffers so RAM use stays deterministic and
// saved work remains compact on the SD card.
class WritingApp final : public UtilityAppBase {
 public:
  AppId id() const override { return AppId::Writing; }
  const char* title() const override { return "Write & Sketch"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 private:
  struct Point { int16_t x; int16_t y; };
  struct Stroke { uint16_t first; uint16_t count; bool eraser; };
  // Natural handwriting consists of many short strokes. These limits allow a
  // full page of writing while retaining deterministic RAM use (~25 KB).
  static constexpr uint8_t kMaxStrokes = 128;
  static constexpr uint16_t kMaxPoints = 6000;
  static constexpr int kCanvasLeft = 15;
  static constexpr int kCanvasRight = 525;
  // Status and the complete equal-size drawing toolbar live above the canvas.
  static constexpr int kCanvasTop = 198;
  void draw(AppContext& context);
  void drawCanvas() const;
  void drawStrokes() const;
  void drawActions() const;
  void drawStatus() const;
  void drawLibrary(AppContext& context);
  void redrawCanvas() const;
  void drawSegment(const Point& from, const Point& to, bool eraser) const;
  void refreshSegment(const Point& from, const Point& to) const;
  void clear();
  void newDrawing();
  void undo();
  void save(AppContext& context);
  void scanDrawings(AppContext& context);
  bool loadDrawing(const String& path);
  bool withinCanvas(int x, int y) const;
  Point points_[kMaxPoints]{};
  Stroke strokes_[kMaxStrokes]{};
  uint16_t pointCount_{0};
  uint8_t strokeCount_{0};
  static constexpr uint8_t kMaxSketches = 24;
  static constexpr uint8_t kSketchesPerPage = 7;
  String sketches_[kMaxSketches];
  uint8_t sketchCount_{0};
  uint8_t sketchPage_{0};
  bool eraser_{false};
  bool drawing_{false};
  bool hasInk_{false};
  bool libraryOpen_{false};
  Point lastPoint_{};
  String savedPath_{};
  String status_{"Pen ready"};
};

class SystemMonitorApp final : public UtilityAppBase {
 public:
  AppId id() const override { return AppId::SystemMonitor; }
  const char* title() const override { return "System Monitor"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
 private:
  void draw(AppContext& context); void drawReadings(AppContext& context); uint32_t lastDrawMs_{0};
};
