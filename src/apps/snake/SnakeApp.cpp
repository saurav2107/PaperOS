#include "apps/snake/SnakeApp.h"

#include <M5Unified.h>
#include <Preferences.h>
#include "app/AppContext.h"
#include "ui/Chrome.h"
#include "ui/Typography.h"
#include "ui/Theme.h"

namespace {
ui::ChromeOptions gameChrome() { ui::ChromeOptions options; options.showHome = true; return options; }
void gameButton(int x, int y, int width, int height, const char* label) {
  ui::Theme::drawButtonFrame(x, y, width, height);
  ui::Typography::apply(ui::TextRole::BodyBold, 0.88f); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(label, x + width / 2, y + height / 2 + 1);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}
}

bool SnakeApp::onStart(AppContext& context) {
  loadHighScore(); reset(); M5.Speaker.setVolume(120); lastStepMs_ = millis(); draw(context); return true;
}

void SnakeApp::loadHighScore() {
  Preferences preferences; preferences.begin("paperos", true);
  highScore_ = preferences.getUShort("snakeHigh", 0);
  preferences.end();
}

void SnakeApp::saveHighScore() const {
  Preferences preferences; preferences.begin("paperos", false);
  preferences.putUShort("snakeHigh", highScore_);
  preferences.end();
}

void SnakeApp::reset() {
  length_ = 4; score_ = 0; gameOver_ = false;
  direction_ = queuedDirection_ = Direction::Right;
  for (uint16_t i = 0; i < length_; ++i) snake_[i] = {static_cast<int8_t>(7 - i), 8};
  placeFood();
}

bool SnakeApp::contains(int x, int y) const {
  for (uint16_t i = 0; i < length_; ++i) if (snake_[i].x == x && snake_[i].y == y) return true;
  return false;
}

void SnakeApp::placeFood() {
  // The board is small and sparse; retrying avoids a dynamically allocated
  // free-cell list while still producing a new valid food position.
  for (uint16_t attempt = 0; attempt < 512; ++attempt) {
    const int x = random(Columns), y = random(Rows);
    if (!contains(x, y)) { food_ = {static_cast<int8_t>(x), static_cast<int8_t>(y)}; return; }
  }
  gameOver_ = true;
}

bool SnakeApp::isOpposite(Direction direction) const {
  return (direction_ == Direction::Up && direction == Direction::Down)
      || (direction_ == Direction::Down && direction == Direction::Up)
      || (direction_ == Direction::Left && direction == Direction::Right)
      || (direction_ == Direction::Right && direction == Direction::Left);
}

void SnakeApp::setDirection(Direction direction) {
  if (!isOpposite(direction)) queuedDirection_ = direction;
}

void SnakeApp::drawCell(int x, int y, bool filled) {
  if (x < 0 || x >= Columns || y < 0 || y >= Rows) return;
  M5.Display.fillRect(Left + x * Cell + 3, Top + y * Cell + 3, Cell - 6, Cell - 6, filled ? TFT_BLACK : TFT_WHITE);
}

void SnakeApp::drawFood() {
  const int x = Left + food_.x * Cell + Cell / 2, y = Top + food_.y * Cell + Cell / 2;
  M5.Display.fillCircle(x, y, 7, TFT_BLACK);
}

void SnakeApp::step() {
  direction_ = queuedDirection_;
  Point head = snake_[0];
  if (direction_ == Direction::Up) --head.y;
  else if (direction_ == Direction::Right) ++head.x;
  else if (direction_ == Direction::Down) ++head.y;
  else --head.x;
  const bool eating = head.x == food_.x && head.y == food_.y;
  // Moving into the current tail is allowed when it will move away this step.
  const bool bodyCollision = contains(head.x, head.y) && !( !eating && head.x == snake_[length_ - 1].x && head.y == snake_[length_ - 1].y );
  if (head.x < 0 || head.x >= Columns || head.y < 0 || head.y >= Rows || bodyCollision) {
    gameOver_ = true; playGameOverSound(); drawStatus(); drawGameOver(); return;
  }
  const Point oldTail = snake_[length_ - 1];
  if (eating && length_ < MaxLength) ++length_;
  for (uint16_t i = length_ - 1; i > 0; --i) snake_[i] = snake_[i - 1];
  snake_[0] = head;
  M5.Display.startWrite();
  if (!eating) drawCell(oldTail.x, oldTail.y, false);
  drawCell(head.x, head.y, true);
  M5.Display.endWrite();
  if (eating) {
    score_ += 10;
    if (score_ > highScore_) { highScore_ = score_; saveHighScore(); }
    playTone(920, 90); placeFood(); drawFood(); drawStatus();
  }
}

void SnakeApp::drawInitialBoard() {
  M5.Display.fillRect(Left - 3, Top - 3, Columns * Cell + 6, Rows * Cell + 6, TFT_BLACK);
  M5.Display.fillRect(Left, Top, Columns * Cell, Rows * Cell, TFT_WHITE);
  for (uint16_t i = 0; i < length_; ++i) drawCell(snake_[i].x, snake_[i].y, true);
  drawFood();
}

void SnakeApp::drawStatus() {
  M5.Display.fillRect(144, 96, 252, 42, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::BodyBold, 0.84f); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(String("SCORE ") + score_ + "  HI " + highScore_, 152, 117);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void SnakeApp::drawControls() {
  M5.Display.fillRect(18, 780, 504, 92, TFT_WHITE);
  const char* labels[] = {"LEFT", "UP", "RIGHT", "DOWN"};
  for (int i = 0; i < 4; ++i) gameButton(18 + i * 130, 780, 114, 64, labels[i]);
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("SWIPE BOARD OR USE CONTROLS", 270, 856);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void SnakeApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Snake"); ui::Chrome::drawFooter(gameChrome());
  M5.Display.fillRoundRect(18, 82, 504, 674, 12, TFT_BLACK); M5.Display.fillRoundRect(21, 85, 498, 668, 9, TFT_WHITE);
  gameButton(30, 94, 108, 44, "RESET");
  drawStatus(); drawInitialBoard(); drawControls();
}

void SnakeApp::drawGameOver() {
  const int x = Left + 42, y = Top + (Rows * Cell) / 2 - 42, width = Columns * Cell - 84;
  M5.Display.fillRoundRect(x, y, width, 84, 10, TFT_BLACK); M5.Display.fillRoundRect(x + 3, y + 3, width - 6, 78, 7, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::Heading, 0.90f); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("GAME OVER", x + width / 2, y + 30);
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.drawString("TAP RESET TO PLAY AGAIN", x + width / 2, y + 60);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void SnakeApp::playTone(uint16_t frequency, uint16_t durationMs) const { M5.Speaker.tone(frequency, durationMs); }

void SnakeApp::playGameOverSound() const {
  // A brief descending arcade-style ending. This runs only after a terminal
  // collision, so its small blocking duration cannot affect game input.
  const uint16_t notes[] = {784, 659, 523, 392};
  const uint16_t lengths[] = {120, 120, 150, 360};
  for (uint8_t index = 0; index < 4; ++index) {
    M5.Speaker.tone(notes[index], lengths[index]);
    delay(lengths[index] + 20);
  }
}

void SnakeApp::onTick(AppContext& context, uint32_t nowMs) {
  const auto& touch = M5.Touch.getDetail();
  if (touch.wasPressed()) {
    if (ui::Chrome::hitTestFooter(touch.x, touch.y, gameChrome()) != ui::FooterAction::None) { if (navigator_) navigator_(AppId::Launcher); return; }
    if (touch.x >= 30 && touch.x <= 138 && touch.y >= 94 && touch.y <= 138) { reset(); playTone(720, 70); lastStepMs_ = nowMs; draw(context); return; }
    if (!gameOver_ && touch.y >= 780 && touch.y <= 844) {
      const int button = touch.x / 130;
      if (button == 0) setDirection(Direction::Left);
      else if (button == 1) setDirection(Direction::Up);
      else if (button == 2) setDirection(Direction::Right);
      else setDirection(Direction::Down);
      playTone(400, 25);
    }
  }
  if (!gameOver_ && touch.wasReleased() && touch.base_x >= Left && touch.base_x < Left + Columns * Cell && touch.base_y >= Top && touch.base_y < Top + Rows * Cell) {
    const int dx = touch.distanceX(), dy = touch.distanceY();
    if (abs(dx) >= 32 || abs(dy) >= 32) {
      if (abs(dx) > abs(dy)) setDirection(dx < 0 ? Direction::Left : Direction::Right);
      else setDirection(dy < 0 ? Direction::Up : Direction::Down);
      playTone(400, 25);
    }
  }
  if (!gameOver_ && nowMs - lastStepMs_ >= 620) { step(); lastStepMs_ = nowMs; }
}
