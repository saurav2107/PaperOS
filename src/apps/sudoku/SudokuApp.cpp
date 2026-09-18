#include "apps/sudoku/SudokuApp.h"

#include <M5Unified.h>
#include "app/AppContext.h"
#include "ui/Chrome.h"
#include "ui/Typography.h"

namespace {
constexpr uint8_t kPuzzle[9][9] = {
  {5,3,0,0,7,0,0,0,0}, {6,0,0,1,9,5,0,0,0}, {0,9,8,0,0,0,0,6,0},
  {8,0,0,0,6,0,0,0,3}, {4,0,0,8,0,3,0,0,1}, {7,0,0,0,2,0,0,0,6},
  {0,6,0,0,0,0,2,8,0}, {0,0,0,4,1,9,0,0,5}, {0,0,0,0,8,0,0,7,9}
};
}

int SudokuApp::cellSize() const { return (M5.Display.width() - 48) / 9; }
int SudokuApp::boardLeft() const { return (M5.Display.width() - cellSize() * 9) / 2; }
int SudokuApp::boardTop() const { return ui::Chrome::headerHeight() + 42; }

bool SudokuApp::onStart(AppContext& context) { reset(); draw(context); return true; }

void SudokuApp::reset() {
  for (int row = 0; row < 9; ++row) for (int col = 0; col < 9; ++col) {
    values_[row][col] = kPuzzle[row][col];
    fixed_[row][col] = kPuzzle[row][col] != 0;
  }
  selectedRow_ = selectedCol_ = -1;
  status_ = "SELECT AN EMPTY CELL";
}

bool SudokuApp::valid(int row, int col, uint8_t value) const {
  if (!value) return true;
  for (int i = 0; i < 9; ++i) {
    if (i != col && values_[row][i] == value) return false;
    if (i != row && values_[i][col] == value) return false;
  }
  const int baseRow = (row / 3) * 3, baseCol = (col / 3) * 3;
  for (int r = baseRow; r < baseRow + 3; ++r) for (int c = baseCol; c < baseCol + 3; ++c)
    if ((r != row || c != col) && values_[r][c] == value) return false;
  return true;
}

bool SudokuApp::solved() const {
  for (int row = 0; row < 9; ++row) for (int col = 0; col < 9; ++col)
    if (!values_[row][col] || !valid(row, col, values_[row][col])) return false;
  return true;
}

void SudokuApp::setValue(uint8_t value) {
  if (selectedRow_ < 0 || fixed_[selectedRow_][selectedCol_]) { status_ = "SELECT AN EMPTY CELL"; return; }
  if (!valid(selectedRow_, selectedCol_, value)) { status_ = "THAT NUMBER DOES NOT FIT"; return; }
  values_[selectedRow_][selectedCol_] = value;
  status_ = solved() ? "PUZZLE COMPLETE!" : "GOOD MOVE";
}

void SudokuApp::drawBoard() {
  const int cell=cellSize(), left=boardLeft(), top=boardTop(), size=cell*9;
  M5.Display.fillRect(left - 3, top - 3, size + 6, size + 6, TFT_BLACK);
  M5.Display.fillRect(left, top, size, size, TFT_WHITE);
  for (int i = 1; i < 9; ++i) {
    // One-pixel rules become inconsistent after an e-paper partial refresh.
    // Keep regular cell lines at 2px and 3x3 boundaries at 3px.
    const int thickness = i % 3 == 0 ? 3 : 2;
    M5.Display.fillRect(left + i * cell - thickness / 2, top, thickness, size, TFT_BLACK);
    M5.Display.fillRect(left, top + i * cell - thickness / 2, size, thickness, TFT_BLACK);
  }
  if (selectedRow_ >= 0) {
    M5.Display.fillRoundRect(left + selectedCol_ * cell + 4, top + selectedRow_ * cell + 4, cell - 8, cell - 8, 5, TFT_BLACK);
  }
  for (int row = 0; row < 9; ++row) for (int col = 0; col < 9; ++col) {
    if (!values_[row][col]) continue;
    const String text(values_[row][col]);
    const bool selected = row == selectedRow_ && col == selectedCol_;
    M5.Display.setTextColor(selected ? TFT_WHITE : TFT_BLACK, selected ? TFT_BLACK : TFT_WHITE);
    ui::Typography::apply(fixed_[row][col] ? ui::TextRole::BodyBold : ui::TextRole::Body, 1.15f);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(text, left + col * cell + cell / 2, top + row * cell + cell / 2 + 1);
  }
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE); M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void SudokuApp::drawKeypad() {
  const int actionTop=boardTop()+cellSize()*9+16;
  const int keypadTop=actionTop+60, keyW=130, keyH=58, keyGap=20;
  M5.Display.fillRect(18, actionTop, M5.Display.width()-36, ui::Chrome::footerTop()-actionTop, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.setTextDatum(ML_DATUM); M5.Display.drawString(status_, 28, actionTop+21);
  auto action=[actionTop](int x,const char* label){M5.Display.fillRoundRect(x,actionTop,92,42,7,TFT_BLACK);M5.Display.fillRoundRect(x+3,actionTop+3,86,36,5,TFT_WHITE);ui::Typography::apply(ui::TextRole::Caption);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString(label,x+46,actionTop+22);};
  action(314,"CLEAR"); action(416,"NEW");
  for (int i = 0; i < 9; ++i) {
    const int row = i / 3, col = i % 3, x = (M5.Display.width() - (keyW * 3 + keyGap * 2)) / 2 + col * (keyW + keyGap), y = keypadTop + row * (keyH + 12);
    M5.Display.fillRoundRect(x, y, keyW, keyH, 8, TFT_BLACK); M5.Display.fillRoundRect(x+3, y+3, keyW-6, keyH-6, 5, TFT_WHITE);
    ui::Typography::apply(ui::TextRole::BodyBold, 1.25f); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(String(i + 1), x + keyW/2, y + keyH/2 + 1);
  }
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void SudokuApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Sudoku");
  drawBoard(); drawKeypad(); ui::ChromeOptions chrome; chrome.showHome=true; ui::Chrome::drawFooter(chrome);
}

void SudokuApp::onTick(AppContext& context, uint32_t) {
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed()) return;
  ui::ChromeOptions chrome; chrome.showHome=true;
  if (ui::Chrome::hitTestFooter(touch.x, touch.y, chrome) != ui::FooterAction::None) {
    if (navigator_) navigator_(AppId::Launcher); return;
  }
  const int cell=cellSize(), left=boardLeft(), top=boardTop(), actionTop=top+cell*9+16, keypadTop=actionTop+60;
  if (touch.x >= left && touch.x < left + cell * 9 && touch.y >= top && touch.y < top + cell * 9) {
    const int row = (touch.y - top) / cell, col = (touch.x - left) / cell;
    if (fixed_[row][col]) status_ = "THAT IS A FIXED NUMBER";
    else { selectedRow_ = row; selectedCol_ = col; status_ = "CHOOSE A NUMBER"; }
    draw(context); return;
  }
  if (touch.y >= actionTop && touch.y <= actionTop+42 && touch.x >= 314 && touch.x <= 406) { setValue(0); draw(context); return; }
  if (touch.y >= actionTop && touch.y <= actionTop+42 && touch.x >= 416 && touch.x <= 508) { reset(); draw(context); return; }
  const int keyW=130,keyH=58,keyGap=20,keyLeft=(M5.Display.width()-(keyW*3+keyGap*2))/2;
  if (touch.x >= keyLeft && touch.x < keyLeft + keyW*3 + keyGap*2 && touch.y >= keypadTop && touch.y < keypadTop + (keyH+12)*3) {
    if ((touch.x-keyLeft)%(keyW+keyGap)>=keyW || (touch.y-keypadTop)%(keyH+12)>=keyH) return;
    const int col = (touch.x - keyLeft) / (keyW+keyGap), row = (touch.y - keypadTop) / (keyH+12);
    if (col >= 0 && col < 3 && row >= 0 && row < 3) setValue(static_cast<uint8_t>(row * 3 + col + 1));
    draw(context);
  }
}
