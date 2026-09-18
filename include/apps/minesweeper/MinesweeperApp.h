#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// A fixed beginner board deliberately avoids heap allocation. The same 81
// cells also bound the flood-fill queue, which keeps gameplay predictable on
// an embedded target.
class MinesweeperApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Minesweeper; }
  const char* title() const override { return "Minesweeper"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override {}

 private:
  static constexpr uint8_t Columns = 9;
  static constexpr uint8_t Rows = 9;
  static constexpr uint8_t MineCount = 10;
  static constexpr int Cell = 50;
  static constexpr int Left = 45;
  static constexpr int Top = 166;
  enum class State : uint8_t { Ready, Playing, Won, Lost };

  void reset();
  void placeMines(uint8_t safeRow, uint8_t safeColumn);
  uint8_t countAround(uint8_t row, uint8_t column) const;
  void reveal(uint8_t row, uint8_t column);
  void revealAllMines();
  void draw(AppContext& context);
  void drawBoard();
  void drawCell(uint8_t row, uint8_t column);
  void drawStatus();
  void drawControls();
  void drawResetConfirmation();
  void refreshCell(uint8_t row, uint8_t column);
  void refreshStatus();
  bool inBoard(int x, int y) const;

  bool mines_[Rows][Columns]{};
  bool revealed_[Rows][Columns]{};
  bool flagged_[Rows][Columns]{};
  uint8_t around_[Rows][Columns]{};
  uint8_t flags_{0};
  uint8_t revealedSafe_{0};
  bool minesPlaced_{false};
  bool flagMode_{false};
  bool resetConfirmation_{false};
  State state_{State::Ready};
  NavigationCallback navigator_{nullptr};
};
