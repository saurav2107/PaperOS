#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// The puzzle is completely static and allocation-free.  It provides a small,
// readable touch UI without holding resources while another app is active.
class SudokuApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Sudoku; }
  const char* title() const override { return "Sudoku"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override { selectedRow_ = selectedCol_ = -1; }

 private:
  void reset();
  bool valid(int row, int col, uint8_t value) const;
  bool solved() const;
  void setValue(uint8_t value);
  void draw(AppContext& context);
  void drawBoard();
  void drawKeypad();
  int cellSize() const;
  int boardLeft() const;
  int boardTop() const;

  uint8_t values_[9][9]{};
  bool fixed_[9][9]{};
  int8_t selectedRow_{-1};
  int8_t selectedCol_{-1};
  String status_;
  NavigationCallback navigator_{nullptr};
};
