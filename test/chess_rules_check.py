"""Host regression check of the actual ChessApp rule methods (no ESP32 needed).

Run: python3 test/chess_rules_check.py
The small host shim replaces Arduino/UI declarations, not the move rules.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
header = (root / 'include/apps/chess/ChessApp.h').read_text()
header = '\n'.join(line for line in header.splitlines() if not line.startswith('#'))
header = header.replace(' final : public IApp', '').replace(' override', '').replace('private:', 'public:')
source = (root / 'src/apps/chess/ChessApp.cpp').read_text()
names = ['isWhite', 'isBlack', 'sameSide', 'reset', 'pathClear', 'canMove',
         'attacksSquare', 'isKingInCheck', 'isLegalMove', 'pieceValue', 'move',
         'hasLegalMove', 'updateTurnStatus', 'respond']
methods = []
for name in names:
    marker = 'ChessApp::' + name + '('
    start = source.rfind('\n', 0, source.index(marker)) + 1
    end = source.index('\n}', source.index(marker)) + 2
    # The first three helpers are single-line definitions.
    if name in ['isWhite', 'isBlack']:
        end = source.index('\n', start)
    methods.append(source[start:end])
shim = '''#include <string>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <iostream>
using String = std::string;
enum class AppId { Chess };
struct AppContext {};
using NavigationCallback = void(*)(AppId);
'''
tests = r'''
int main() {
  using C = ChessApp;
  C c;
  c.reset();
  int count=0;
  for(int a=0;a<8;++a)for(int b=0;b<8;++b)
    for(int x=0;x<8;++x)for(int y=0;y<8;++y)
      count+=c.isLegalMove(a,b,x,y);
  assert(count==20);
  assert(!c.isLegalMove(7,0,5,0)); // Blocked rook
  c.move({6,4,4,4,C::Empty}); c.whiteToMove_=false;
  C::Move reply{}; assert(c.respond(reply));
  assert(c.whiteToMove_ && !c.isKingInCheck(false));
  const auto clear=[&] { for(auto& r:c.board_)for(auto& p:r)p=C::Empty; c.gameOver_=false; };
  clear(); c.board_[7][4]=C::WKing; c.board_[0][0]=C::BKing;
  c.board_[0][4]=C::BRook; c.board_[6][4]=C::WRook;
  assert(!c.isLegalMove(6,4,6,5)); // Pinned rook
  assert(c.board_[6][4]==C::WRook && c.board_[6][5]==C::Empty);
  clear(); c.board_[7][7]=C::WKing; c.board_[5][5]=C::BKing; c.board_[6][6]=C::BQueen;
  c.whiteToMove_=true; c.updateTurnStatus();
  assert(c.gameOver_ && c.status_=="Checkmate: Black wins.");
  clear(); c.board_[0][0]=C::BKing; c.board_[2][2]=C::WKing; c.board_[1][1]=C::WQueen;
  c.whiteToMove_=false; c.updateTurnStatus();
  assert(c.gameOver_ && c.status_=="Checkmate: White wins.");
  clear(); c.board_[0][0]=C::BKing; c.board_[2][2]=C::WKing; c.board_[2][1]=C::WQueen;
  c.updateTurnStatus(); assert(c.gameOver_ && c.status_=="Stalemate.");
  clear(); c.board_[7][7]=C::WKing; c.board_[0][7]=C::BKing; c.board_[6][0]=C::BPawn;
  c.move({6,0,7,0,C::BQueen}); assert(c.board_[7][0]==C::BQueen);
  assert(!c.canMove(7,0,7,7,false)); // Cannot capture king
  std::cout << "Chess rule regressions passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='paperos-chess-') as directory:
    cpp = Path(directory) / 'rules.cpp'
    binary = Path(directory) / 'rules'
    cpp.write_text(shim + header + '\n' + '\n'.join(methods) + tests)
    subprocess.run(['c++', '-std=c++17', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
