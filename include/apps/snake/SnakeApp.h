#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// Fixed arrays give this game deterministic memory use: no heap allocation
// occurs while the snake grows or during rapid touch input.
class SnakeApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Snake; }
  const char* title() const override { return "Snake"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override {}

 private:
  struct Point { int8_t x; int8_t y; };
  enum class Direction : int8_t { Up, Right, Down, Left };
  static constexpr int Columns = 15;
  static constexpr int Rows = 18;
  static constexpr int Cell = 32;
  static constexpr int Left = 30;
  static constexpr int Top = 160;
  static constexpr int MaxLength = Columns * Rows;

  void reset();
  void placeFood();
  bool contains(int x, int y) const;
  bool isOpposite(Direction direction) const;
  void setDirection(Direction direction);
  void step();
  void draw(AppContext& context);
  void drawInitialBoard();
  void drawCell(int x, int y, bool filled);
  void drawFood();
  void drawStatus();
  void drawControls();
  void drawGameOver();
  void playTone(uint16_t frequency, uint16_t durationMs) const;
  void playGameOverSound() const;
  void loadHighScore();
  void saveHighScore() const;

  Point snake_[MaxLength]{};
  uint16_t length_{0};
  Point food_{};
  Direction direction_{Direction::Right};
  Direction queuedDirection_{Direction::Right};
  uint16_t score_{0};
  uint16_t highScore_{0};
  bool gameOver_{false};
  uint32_t lastStepMs_{0};
  NavigationCallback navigator_{nullptr};
};
