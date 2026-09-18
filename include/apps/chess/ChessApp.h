#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"

// A PaperS3-aware adaptation of arunmathaisk/PaperS3-chess.  The game data is
// deliberately owned by this app, so leaving Chess frees all app state and
// does not re-initialize the shared display, touch, or SD services.
class ChessApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Chess; }
  const char* title() const override { return "Chess"; }
  bool onStart(AppContext&) override;
  void onTick(AppContext&, uint32_t) override;
  void onStop(AppContext&) override;

 private:
  enum Piece : int8_t {
    Empty, WPawn, WRook, WKnight, WBishop, WQueen, WKing,
    BPawn, BRook, BKnight, BBishop, BQueen, BKing
  };
  struct Move { int8_t fromRow, fromCol, toRow, toCol; Piece promotion; };

  static constexpr int BoardTop = 96;
  static constexpr int Square = 61;
  static constexpr int BoardLeft = 26;

  void reset();
  void draw(AppContext&);
  void drawBoard();
  void drawSquare(int row, int col);
  void refreshSquare(int row, int col);
  void refreshStatus();
  void drawPiece(int row, int col);
  void drawStatus();
  void drawStatusText();
  bool hasLegalMove(bool white);
  void updateTurnStatus();
  void drawNewGameConfirmation(AppContext&);
  void handleBoardTap(AppContext&, int row, int col);
  bool canMove(int fr, int fc, int tr, int tc, bool enforceTurn = true) const;
  bool isLegalMove(int fr, int fc, int tr, int tc, bool enforceTurn = true);
  bool pathClear(int fr, int fc, int tr, int tc) const;
  bool attacksSquare(int fr, int fc, int tr, int tc) const;
  bool isKingInCheck(bool whiteKing) const;
  int pieceValue(Piece piece) const;
  bool isWhite(Piece piece) const;
  bool isBlack(Piece piece) const;
  bool sameSide(Piece a, Piece b) const;
  void move(const Move& move);
  bool respond(Move& response);
  bool assetAvailable_{false};
  bool whiteToMove_{true};
  bool gameOver_{false};
  bool selected_{false};
  bool newGameConfirmationVisible_{false};
  bool computerMovePending_{false};
  uint32_t computerMoveAtMs_{0};
  int8_t selectedRow_{-1}, selectedCol_{-1};
  Piece board_[8][8]{};
  String status_;
  String renderedStatus_;
  NavigationCallback navigator_{nullptr};
};
