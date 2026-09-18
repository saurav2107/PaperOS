#include "apps/chess/ChessApp.h"

#include <SD.h>
#include <M5Unified.h>
#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Popup.h"
#include "ui/Typography.h"

namespace {
constexpr uint16_t kLightSquare = TFT_WHITE;
constexpr uint16_t kDarkSquare = TFT_DARKGREY;
const char* const kPieceFiles[] = {
  nullptr, "white_pawn.png", "white_rook.png", "white_knight.png",
  "white_bishop.png", "white_queen.png", "white_king.png",
  "black_pawn.png", "black_rook.png", "black_knight.png",
  "black_bishop.png", "black_queen.png", "black_king.png"
};
}

bool ChessApp::isWhite(Piece p) const { return p >= WPawn && p <= WKing; }
bool ChessApp::isBlack(Piece p) const { return p >= BPawn && p <= BKing; }
bool ChessApp::sameSide(Piece a, Piece b) const {
  return a != Empty && b != Empty && (isWhite(a) == isWhite(b));
}

bool ChessApp::onStart(AppContext& context) {
  assetAvailable_ = context.storage.mounted() && SD.exists("/PaperOS/chess/white_king.png");
  reset();
  draw(context);
  return true;
}

void ChessApp::reset() {
  for (auto& row : board_) for (Piece& p : row) p = Empty;
  const Piece back[] = {BRook, BKnight, BBishop, BQueen, BKing, BBishop, BKnight, BRook};
  const Piece whiteBack[] = {WRook, WKnight, WBishop, WQueen, WKing, WBishop, WKnight, WRook};
  for (int col = 0; col < 8; ++col) {
    board_[0][col] = back[col]; board_[1][col] = BPawn;
    board_[6][col] = WPawn; board_[7][col] = whiteBack[col];
  }
  whiteToMove_ = true; selected_ = false; selectedRow_ = selectedCol_ = -1;
  computerMovePending_ = false; computerMoveAtMs_ = 0;
  gameOver_ = false; renderedStatus_ = "";
  status_ = assetAvailable_ ? "Your turn: play White." : "Piece images missing: copy assets to /PaperOS/chess.";
}

void ChessApp::drawBoard() {
  for (int row = 0; row < 8; ++row) for (int col = 0; col < 8; ++col) drawSquare(row, col);
}

void ChessApp::drawSquare(int row, int col) {
  const int x = BoardLeft + col * Square, y = BoardTop + row * Square;
  const uint16_t colour = ((row + col) & 1) ? kDarkSquare : kLightSquare;
  M5.Display.fillRect(x, y, Square, Square, colour);
  M5.Display.drawRect(x, y, Square, Square, TFT_BLACK);
  if (selected_ && row == selectedRow_ && col == selectedCol_) {
    // Ink-only highlight remains visible on the PaperS3; red is not part of
    // the e-paper palette and previously made selection feedback unreliable.
    M5.Display.drawRect(x + 2, y + 2, Square - 4, Square - 4, TFT_BLACK);
    M5.Display.drawRect(x + 4, y + 4, Square - 8, Square - 8, TFT_BLACK);
  }
  drawPiece(row, col);
}

void ChessApp::refreshSquare(int row, int col) {
  drawSquare(row, col);
  M5.Display.display(BoardLeft + col * Square, BoardTop + row * Square, Square, Square);
}

void ChessApp::refreshStatus() {
  if (renderedStatus_ == status_) return;
  drawStatusText();
  M5.Display.display(42, 660, 456, 54);
}

void ChessApp::drawPiece(int row, int col) {
  const Piece piece = board_[row][col];
  if (piece == Empty) return;
  const int x = BoardLeft + col * Square + 2, y = BoardTop + row * Square + 2;
  if (assetAvailable_) {
    File png = SD.open(String("/PaperOS/chess/") + kPieceFiles[piece]);
    if (png) { M5.Display.drawPng(&png, x, y); png.close(); return; }
  }
  // A legible fallback permits testing without a microSD card.
  const char glyphs[] = {' ', 'P', 'R', 'N', 'B', 'Q', 'K', 'p', 'r', 'n', 'b', 'q', 'k'};
  ui::Typography::apply(ui::TextRole::Title, 1.25f); M5.Display.setTextColor(TFT_BLACK);
  M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString(String(glyphs[piece]), x + Square / 2 - 2, y + Square / 2);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void ChessApp::drawStatus() {
  const int top = 602, bottom = ui::Chrome::footerTop() - 16, height = bottom - top;
  M5.Display.fillRect(18, top, 504, height, TFT_WHITE);
  M5.Display.fillRoundRect(18, top, 504, height, 10, TFT_BLACK);
  M5.Display.fillRoundRect(21, top + 3, 498, height - 6, 7, TFT_WHITE);
  M5.Display.fillRect(30, bottom - 3, 480, 3, TFT_BLACK);
  ui::Typography::apply(ui::TextRole::Heading); M5.Display.setTextDatum(ML_DATUM); M5.Display.drawString("CHESS", 42, top + 36);
  drawStatusText();
  M5.Display.setTextDatum(ML_DATUM);
  ui::Typography::apply(ui::TextRole::Caption); M5.Display.drawString("Tap a White piece, then its destination.", 42, top + 128);
  M5.Display.drawString("PaperOS responds as Black after a valid move.", 42, top + 152);
  const int buttonY = bottom - 66;
  M5.Display.fillRoundRect(170, buttonY, 200, 52, 8, TFT_BLACK);
  M5.Display.fillRoundRect(173, buttonY + 3, 194, 46, 5, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::BodyBold); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("NEW GAME", 270, buttonY + 27);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void ChessApp::drawStatusText() {
  const int top = 602;
  M5.Display.fillRect(42,660,456,54,TFT_WHITE);
  M5.Display.setClipRect(42,660,456,54);
  M5.Display.setTextColor(TFT_BLACK,TFT_WHITE);
  M5.Display.setTextDatum(ML_DATUM);
  ui::Typography::apply(ui::TextRole::Body, 0.90f);
  String firstLine=status_, secondLine;
  while (!firstLine.isEmpty() && M5.Display.textWidth(firstLine) > 438) {
    const int split=firstLine.lastIndexOf(' '); if(split<1)break;
    secondLine=firstLine.substring(split+1)+(secondLine.isEmpty()?"":" "+secondLine); firstLine.remove(split);
  }
  M5.Display.drawString(firstLine, 42, top + 76);
  if (!secondLine.isEmpty()) M5.Display.drawString(secondLine, 42, top + 100);
  M5.Display.clearClipRect();
  renderedStatus_ = status_;
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

void ChessApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE);
  ui::Chrome::drawHeader(context, "Chess");
  drawBoard(); drawStatus(); ui::ChromeOptions chrome; chrome.showHome = true; ui::Chrome::drawFooter(chrome);
}

void ChessApp::drawNewGameConfirmation(AppContext& context) {
  // Blocking confirmation: the current board is intentionally not redrawn
  // behind the dialog, so a tap cannot accidentally play a move.
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Chess");
  const int x = 38, y = 270, width = M5.Display.width() - 76, height = 330;
  ui::Popup::drawFrame(x, y, width, height, "START NEW GAME?");
  M5.Display.fillRect(x + 14, y + height - 3, width - 28, 3, TFT_BLACK);
  ui::Typography::apply(ui::TextRole::Body, 0.90f); M5.Display.setTextDatum(MC_DATUM); M5.Display.drawString("Your current board position", 270, y + 128);
  M5.Display.drawString("will be discarded.", 270, y + 162);
  const int buttonY = y + 228;
  M5.Display.fillRoundRect(70, buttonY, 172, 58, 8, TFT_BLACK); M5.Display.fillRoundRect(73, buttonY + 3, 166, 52, 5, TFT_WHITE);
  M5.Display.fillRoundRect(298, buttonY, 172, 58, 8, TFT_BLACK); M5.Display.fillRoundRect(301, buttonY + 3, 166, 52, 5, TFT_WHITE);
  ui::Typography::apply(ui::TextRole::BodyBold, 0.90f); M5.Display.drawString("NO", 156, buttonY + 30); M5.Display.drawString("YES", 384, buttonY + 30);
  M5.Display.setTextDatum(TL_DATUM); ui::Typography::reset();
}

bool ChessApp::pathClear(int fr, int fc, int tr, int tc) const {
  const int dr = (tr > fr) - (tr < fr), dc = (tc > fc) - (tc < fc);
  for (int row = fr + dr, col = fc + dc; row != tr || col != tc; row += dr, col += dc)
    if (board_[row][col] != Empty) return false;
  return true;
}

bool ChessApp::canMove(int fr, int fc, int tr, int tc, bool enforceTurn) const {
  if (fr < 0 || fr > 7 || fc < 0 || fc > 7 || tr < 0 || tr > 7 || tc < 0 || tc > 7) return false;
  const Piece piece = board_[fr][fc], target = board_[tr][tc];
  if (piece == Empty || sameSide(piece, target)) return false;
  // Kings are never captured in chess. Check/checkmate are represented by
  // legal-move availability, not by allowing a king to disappear.
  if (target == WKing || target == BKing) return false;
  if (enforceTurn && (whiteToMove_ != isWhite(piece))) return false;
  const int dr = tr - fr, dc = tc - fc, ar = abs(dr), ac = abs(dc);
  if (piece == WPawn || piece == BPawn) {
    const int direction = isWhite(piece) ? -1 : 1, start = isWhite(piece) ? 6 : 1;
    if (dc == 0 && dr == direction && target == Empty) return true;
    if (dc == 0 && dr == 2 * direction && fr == start && target == Empty && board_[fr + direction][fc] == Empty) return true;
    return ac == 1 && dr == direction && target != Empty;
  }
  if (piece == WKnight || piece == BKnight) return (ar == 2 && ac == 1) || (ar == 1 && ac == 2);
  if (piece == WBishop || piece == BBishop) return ar == ac && pathClear(fr, fc, tr, tc);
  if (piece == WRook || piece == BRook) return (ar == 0 || ac == 0) && pathClear(fr, fc, tr, tc);
  if (piece == WQueen || piece == BQueen) return (ar == ac || ar == 0 || ac == 0) && pathClear(fr, fc, tr, tc);
  return ar <= 1 && ac <= 1;
}

bool ChessApp::attacksSquare(int fr, int fc, int tr, int tc) const {
  if (fr == tr && fc == tc) return false;
  if (fr < 0 || fr > 7 || fc < 0 || fc > 7 || tr < 0 || tr > 7 || tc < 0 || tc > 7) return false;
  const Piece piece = board_[fr][fc];
  if (piece == Empty) return false;
  const int dr = tr - fr, dc = tc - fc, ar = abs(dr), ac = abs(dc);
  if (piece == WPawn || piece == BPawn) return ar == 1 && ac == 1 && dr == (isWhite(piece) ? -1 : 1);
  if (piece == WKnight || piece == BKnight) return (ar == 2 && ac == 1) || (ar == 1 && ac == 2);
  if (piece == WBishop || piece == BBishop) return ar == ac && pathClear(fr, fc, tr, tc);
  if (piece == WRook || piece == BRook) return (ar == 0 || ac == 0) && pathClear(fr, fc, tr, tc);
  if (piece == WQueen || piece == BQueen) return (ar == ac || ar == 0 || ac == 0) && pathClear(fr, fc, tr, tc);
  return ar <= 1 && ac <= 1;
}

bool ChessApp::isKingInCheck(bool whiteKing) const {
  const Piece king = whiteKing ? WKing : BKing;
  int kingRow = -1, kingCol = -1;
  for (int row = 0; row < 8; ++row) for (int col = 0; col < 8; ++col)
    if (board_[row][col] == king) { kingRow = row; kingCol = col; }
  if (kingRow < 0) return true;
  for (int row = 0; row < 8; ++row) for (int col = 0; col < 8; ++col) {
    const Piece piece = board_[row][col];
    if (piece != Empty && isWhite(piece) != whiteKing && attacksSquare(row, col, kingRow, kingCol)) return true;
  }
  return false;
}

bool ChessApp::isLegalMove(int fr, int fc, int tr, int tc, bool enforceTurn) {
  if (!canMove(fr, fc, tr, tc, enforceTurn)) return false;
  const Piece moving = board_[fr][fc], captured = board_[tr][tc];
  board_[tr][tc] = moving; board_[fr][fc] = Empty;
  const bool illegal = isKingInCheck(isWhite(moving));
  board_[fr][fc] = moving; board_[tr][tc] = captured;
  return !illegal;
}

int ChessApp::pieceValue(Piece piece) const {
  if (piece == WPawn || piece == BPawn) return 100;
  if (piece == WKnight || piece == BKnight || piece == WBishop || piece == BBishop) return 320;
  if (piece == WRook || piece == BRook) return 500;
  if (piece == WQueen || piece == BQueen) return 900;
  return 0;
}

void ChessApp::move(const Move& move) {
  Piece moving = board_[move.fromRow][move.fromCol];
  board_[move.toRow][move.toCol] = move.promotion == Empty ? moving : move.promotion;
  board_[move.fromRow][move.fromCol] = Empty;
}

bool ChessApp::hasLegalMove(bool white) {
  for (int fr=0;fr<8;++fr) for(int fc=0;fc<8;++fc) {
    if(board_[fr][fc]==Empty || isWhite(board_[fr][fc])!=white) continue;
    for(int tr=0;tr<8;++tr) for(int tc=0;tc<8;++tc)
      if(isLegalMove(fr,fc,tr,tc,false)) return true;
  }
  return false;
}

void ChessApp::updateTurnStatus() {
  const bool check = isKingInCheck(whiteToMove_);
  gameOver_ = !hasLegalMove(whiteToMove_);
  if(gameOver_) {
    computerMovePending_ = false;
    status_ = check ? (whiteToMove_ ? "Checkmate: Black wins." : "Checkmate: White wins.") : "Stalemate.";
  } else status_ = whiteToMove_ ? (check ? "White is in check. Your turn." : "Your turn: play White.") : "Computer is thinking...";
}

bool ChessApp::respond(Move& response) {
  // Evaluate every legal Black reply rather than taking the first geometric
  // move. This keeps the computer out of check, prioritises captures, and
  // avoids the previous "no move"/invisible instant-response behaviour.
  bool found = false;
  int bestScore = -100000;
  for (int fr = 0; fr < 8; ++fr) for (int fc = 0; fc < 8; ++fc) {
    if (!isBlack(board_[fr][fc])) continue;
    for (int tr = 0; tr < 8; ++tr) for (int tc = 0; tc < 8; ++tc) {
      if (!isLegalMove(fr, fc, tr, tc, false)) continue;
      const Piece moving = board_[fr][fc], captured = board_[tr][tc];
      // Captures dominate; centre advancement breaks ties so the computer
      // develops instead of repeatedly pushing the first pawn it finds.
      int score = pieceValue(captured) * 10 + tr * 6 - abs(tc - 3) * 2;
      if (moving == BPawn && tr == 7) score += 800;
      if (!found || score > bestScore) {
        bestScore = score; found = true;
        response = {(int8_t)fr, (int8_t)fc, (int8_t)tr, (int8_t)tc, tr == 7 && moving == BPawn ? BQueen : Empty};
      }
    }
  }
  if (!found) { updateTurnStatus(); return false; }
  move(response); whiteToMove_ = true; updateTurnStatus();
  return true;
}

void ChessApp::handleBoardTap(AppContext& context, int row, int col) {
  if (gameOver_ || !whiteToMove_ || computerMovePending_) return;
  if (!selected_) {
    if (isWhite(board_[row][col])) {
      selected_ = true; selectedRow_ = row; selectedCol_ = col; status_ = "Choose a destination.";
      refreshSquare(row, col);
    } else status_ = "Select a White piece.";
    refreshStatus(); return;
  }
  // Selecting another white piece is a correction, not an invalid move.
  // This avoids the frustrating two-extra-tap sequence from the old logic.
  if (isWhite(board_[row][col])) {
    const int oldRow = selectedRow_, oldCol = selectedCol_;
    if (row == oldRow && col == oldCol) return;
    selectedRow_ = row; selectedCol_ = col; status_ = "Choose a destination.";
    refreshSquare(oldRow, oldCol); refreshSquare(row, col); refreshStatus(); return;
  }
  if (!isLegalMove(selectedRow_, selectedCol_, row, col)) {
    const int oldRow = selectedRow_, oldCol = selectedCol_;
    selected_ = false; selectedRow_ = selectedCol_ = -1; status_ = "That move is not valid.";
    refreshSquare(oldRow, oldCol); refreshStatus(); return;
  }
  const int fromRow = selectedRow_, fromCol = selectedCol_;
  const Piece moving = board_[fromRow][fromCol];
  move({(int8_t)fromRow, (int8_t)fromCol, (int8_t)row, (int8_t)col, row == 0 && moving == WPawn ? WQueen : Empty});
  selected_ = false; selectedRow_ = selectedCol_ = -1; whiteToMove_ = false;
  computerMovePending_ = true; computerMoveAtMs_ = millis() + 350;
  updateTurnStatus();
  // First show the player's legal move. The computer response is scheduled
  // in the next loop pass so e-paper has a distinct, visible turn boundary.
  refreshSquare(fromRow, fromCol); refreshSquare(row, col);
  refreshStatus();
}

void ChessApp::onTick(AppContext& context, uint32_t nowMs) {
  if (!newGameConfirmationVisible_ && computerMovePending_ && static_cast<int32_t>(nowMs - computerMoveAtMs_) >= 0) {
    computerMovePending_ = false;
    Move blackMove{};
    const bool blackMoved = respond(blackMove);
    if (blackMoved) { refreshSquare(blackMove.fromRow, blackMove.fromCol); refreshSquare(blackMove.toRow, blackMove.toCol); }
    refreshStatus();
    return;
  }
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed()) return;
  if (newGameConfirmationVisible_) {
    if (ui::Popup::hitClose(touch.x, touch.y, 38, 270, M5.Display.width() - 76)) { newGameConfirmationVisible_ = false; draw(context); return; }
    if (touch.y >= 498 && touch.y <= 556) {
      if (touch.x >= 70 && touch.x <= 242) { newGameConfirmationVisible_ = false; draw(context); }
      else if (touch.x >= 298 && touch.x <= 470) { newGameConfirmationVisible_ = false; reset(); draw(context); }
    }
    return;
  }
  ui::ChromeOptions chrome; chrome.showHome = true;
  if (ui::Chrome::hitTestFooter(touch.x, touch.y, chrome) != ui::FooterAction::None) {
    if (navigator_) navigator_(AppId::Launcher); return;
  }
  const int newGameTop = ui::Chrome::footerTop() - 82;
  if (touch.x >= 170 && touch.x <= 370 && touch.y >= newGameTop && touch.y <= newGameTop + 52) { newGameConfirmationVisible_ = true; drawNewGameConfirmation(context); return; }
  if (computerMovePending_) return;
  const int col = (touch.x - BoardLeft) / Square, row = (touch.y - BoardTop) / Square;
  if (touch.x >= BoardLeft && col < 8 && touch.y >= BoardTop && row < 8) handleBoardTap(context, row, col);
}

void ChessApp::onStop(AppContext&) { selected_ = false; newGameConfirmationVisible_ = false; computerMovePending_ = false; }
