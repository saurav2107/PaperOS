#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// A compact, allocation-free falling-block game.  Its board and active piece
// live inside the app, so navigation never affects the shared OS services.
class TetrisApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Tetris; }
  const char* title() const override { return "Tetris"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override {}

 private:
  static constexpr int Columns = 10;
  static constexpr int Rows = 16;
  static constexpr int Cell = 36;
  static constexpr int Left = 30;
  static constexpr int Top = 160;

  void reset();
  void spawn();
  bool collides(int piece, int rotation, int originX, int originY) const;
  void rotate();
  void move(int dx);
  bool drop();
  void lockPiece();
  uint8_t clearLines();
  void draw(AppContext& context);
  void drawGameArea(bool refreshStatus = true);
  void drawInitialBoard();
  void drawBoardDelta();
  void drawCell(int column, int row, bool filled);
  void drawPiece(int piece, int rotation, int originX, int originY, bool filled);
  void captureRenderedState();
  void drawStatus();
  void drawNextPiece();
  void drawControls();
  void drawGameOver();
  void playTone(uint16_t frequency, uint16_t durationMs) const;
  void loadHighScore();
  void saveHighScore() const;

  bool board_[Rows][Columns]{};
  bool renderedBoard_[Rows][Columns]{};
  int piece_{0};
  int rotation_{0};
  int pieceX_{3};
  int pieceY_{0};
  uint16_t score_{0};
  uint16_t highScore_{0};
  bool gameOver_{false};
  int previousPiece_{0};
  int previousRotation_{0};
  int previousX_{0};
  int previousY_{0};
  int nextPiece_{1};
  uint32_t lastDropMs_{0};
  NavigationCallback navigator_{nullptr};
};
