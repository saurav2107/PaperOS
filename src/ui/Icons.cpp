#include "ui/Icons.h"

#include <M5Unified.h>

namespace ui {
namespace {
void strokeRect(int x, int y, int w, int h) { M5.Display.drawRect(x, y, w, h, TFT_BLACK); }
void sun(int x, int y, int r) { M5.Display.drawCircle(x,y,r,TFT_BLACK); for(int i=0;i<8;++i){const float a=i*PI/4;M5.Display.drawLine(x+cosf(a)*(r+3),y+sinf(a)*(r+3),x+cosf(a)*(r+8),y+sinf(a)*(r+8),TFT_BLACK);} }
void thickSketchLine(int x0, int y0, int x1, int y1) {
  M5.Display.drawLine(x0, y0, x1, y1, TFT_BLACK);
  M5.Display.drawLine(x0 + 1, y0, x1 + 1, y1, TFT_BLACK);
  M5.Display.drawLine(x0 - 1, y0, x1 - 1, y1, TFT_BLACK);
}
void sketchingMark(int x, int y, int size) {
  // Native monochrome adaptation of ri--sketching: a single expressive,
  // freehand zig-zag stroke rather than the generic pencil used before.
  static constexpr int8_t points[][2] = {{3,18},{5,14},{8,10},{11,7},{13,6},{13,9},{11,14},{10,18},{12,18},{15,15},{18,12},{20,11},{20,14},{18,19},{20,18},{22,15}};
  const float scale = size / 26.0f;
  const int originX = x - static_cast<int>(13 * scale), originY = y - static_cast<int>(13 * scale);
  for (uint8_t i = 1; i < sizeof(points) / sizeof(points[0]); ++i) {
    thickSketchLine(originX + static_cast<int>(points[i - 1][0] * scale), originY + static_cast<int>(points[i - 1][1] * scale),
                    originX + static_cast<int>(points[i][0] * scale), originY + static_cast<int>(points[i][1] * scale));
  }
}
}

void drawIcon(Icon icon, int x, int y, int size) {
  const int r=size/2;
  switch(icon) {
    case Icon::Home:
      M5.Display.fillTriangle(x-r,y-2,x,y-r,x+r,y-2,TFT_BLACK); M5.Display.fillRect(x-r+4,y-2,size-8,r+4,TFT_BLACK); M5.Display.fillRect(x-3,y+7,7,r-5,TFT_WHITE); break;
    case Icon::Weather: sun(x-6,y-5,r/2); M5.Display.fillRoundRect(x-r+4,y+5,size-8,r/2,8,TFT_BLACK); M5.Display.fillCircle(x-r/2,y+5,r/3,TFT_BLACK); break;
    case Icon::Clock:
      // High-contrast analog face: a double ring, filled dial, hour marks,
      // and white hands remain legible on the PaperS3 at small icon sizes.
      M5.Display.drawCircle(x, y, r - 1, TFT_BLACK);
      M5.Display.fillCircle(x, y, r - 4, TFT_BLACK);
      for (int i = 0; i < 12; ++i) { const float a = i * PI / 6.0f - PI / 2.0f; const int mx = x + cosf(a) * (r - 8); const int my = y + sinf(a) * (r - 8); M5.Display.fillCircle(mx, my, size >= 40 ? 2 : 1, TFT_WHITE); }
      M5.Display.drawLine(x, y, x, y - r / 2, TFT_WHITE);
      M5.Display.drawLine(x, y, x + r / 2 - 2, y - 3, TFT_WHITE);
      M5.Display.fillCircle(x, y, size >= 40 ? 3 : 2, TFT_WHITE); break;
    case Icon::Calendar:
      M5.Display.fillRoundRect(x-r,y-r+3,size,size-5,4,TFT_BLACK); M5.Display.fillRect(x-r+4,y-r+15,size-8,size-23,TFT_WHITE); M5.Display.fillRect(x-r/2,y-r,5,13,TFT_BLACK); M5.Display.fillRect(x+r/2-5,y-r,5,13,TFT_BLACK); break;
    case Icon::Book:
      // GG eReader: split device body with a reading pane and text lines.
      M5.Display.fillRoundRect(x-r, y-r+2, size, size-4, 4, TFT_BLACK);
      M5.Display.fillRect(x-r+4, y-r+6, r-5, size-12, TFT_WHITE);
      M5.Display.fillRect(x+2, y-r+6, r-6, size-12, TFT_WHITE);
      for (int i = 0; i < 3; ++i) M5.Display.fillRect(x+5, y-r+9+i*(r/2), r-11, 2, TFT_BLACK);
      M5.Display.fillRect(x-1, y-r+4, 3, size-8, TFT_BLACK); break;
    case Icon::Image:
      // Lineicons Photos: overlapping photo cards, sun, and mountain scene.
      M5.Display.fillRoundRect(x-r+5, y-r+1, size-7, size-7, 4, TFT_BLACK);
      M5.Display.fillRoundRect(x-r+1, y-r+5, size-7, size-7, 4, TFT_BLACK);
      M5.Display.fillRect(x-r+5, y-r+9, size-14, size-15, TFT_WHITE);
      M5.Display.fillCircle(x-r/3, y-r/3+3, 3, TFT_BLACK);
      M5.Display.fillTriangle(x-r+6, y+r-5, x-3, y-1, x+4, y+r-5, TFT_BLACK);
      M5.Display.fillTriangle(x-4, y+r-5, x+8, y-7, x+r-6, y+r-5, TFT_BLACK); break;
    case Icon::Timer:
      // GIS filled timer: bold stopwatch body, crown, button, and timer hand.
      M5.Display.fillCircle(x, y+4, r-4, TFT_BLACK);
      M5.Display.fillRect(x-7, y-r+1, 14, 5, TFT_BLACK);
      M5.Display.fillRect(x-3, y-r+5, 6, 6, TFT_BLACK);
      M5.Display.fillRect(x+r-5, y-r+6, 8, 5, TFT_BLACK);
      M5.Display.drawLine(x, y+4, x, y-r/2+8, TFT_WHITE);
      M5.Display.drawLine(x, y+4, x+r/2-4, y+11, TFT_WHITE);
      M5.Display.fillCircle(x, y+4, 2, TFT_WHITE); break;
    case Icon::Note:
      // Uil Notes: bound notepad with top rings and two writing lines.
      M5.Display.fillRoundRect(x-r+3, y-r+3, size-6, size-6, 4, TFT_BLACK);
      M5.Display.fillRect(x-r+7, y-r+10, size-14, size-15, TFT_WHITE);
      for (int i=0;i<3;++i) M5.Display.fillRect(x-r/2-2+i*(r/2), y-r-5, 3, 9, TFT_BLACK);
      M5.Display.fillRoundRect(x-r/3, y-2, r, 3, 2, TFT_BLACK);
      M5.Display.fillRoundRect(x-r/3, y+8, r+5, 3, 2, TFT_BLACK); break;
    case Icon::Sketch:
      sketchingMark(x, y, size); break;
    case Icon::Games:
      // Fluent Games: rounded controller body, compact D-pad, and two buttons.
      M5.Display.drawRoundRect(x-r, y-r/2, size, r+3, r/2, TFT_BLACK);
      M5.Display.fillRect(x-r/2-3, y-2, 10, 3, TFT_BLACK); M5.Display.fillRect(x-r/2, y-5, 3, 10, TFT_BLACK);
      M5.Display.fillCircle(x+r/3, y+4, 4, TFT_BLACK); M5.Display.fillCircle(x+r/2+2, y-4, 4, TFT_BLACK); break;
    case Icon::HomeAssistant:
      // Official Simple Icons mark: filled home silhouette with automation tree.
      M5.Display.fillTriangle(x-r, y-3, x, y-r, x+r, y-3, TFT_BLACK);
      M5.Display.fillRect(x-r+4, y-3, size-8, r+9, TFT_BLACK);
      M5.Display.drawLine(x, y-r/3, x, y+r/2, TFT_WHITE);
      M5.Display.drawLine(x, y-r/3, x-r/3, y-r/2+4, TFT_WHITE);
      M5.Display.drawLine(x, y+3, x+r/3, y-r/5, TFT_WHITE);
      M5.Display.fillCircle(x-r/3, y-r/2+4, 3, TFT_WHITE); M5.Display.fillCircle(x+r/3, y-r/5, 3, TFT_WHITE);
      M5.Display.fillCircle(x, y+r/2, 3, TFT_WHITE); break;
    case Icon::Folder: M5.Display.fillRoundRect(x-r,y-r/2,size,r+8,3,TFT_BLACK); M5.Display.fillRoundRect(x-r+3,y-r+2,r,9,3,TFT_BLACK); M5.Display.fillRect(x-r+3,y-r/2+4,size-6,r+1,TFT_WHITE); break;
    case Icon::Flashcards:
      // Arcticons flashcards: a lined front card with two offset cards behind.
      M5.Display.drawRoundRect(x-r+5, y-r+3, size-10, size-6, 4, TFT_BLACK);
      M5.Display.drawRoundRect(x-r+1, y-r+7, size-10, size-6, 4, TFT_BLACK);
      M5.Display.drawRoundRect(x-r+9, y-r+7, size-10, size-6, 4, TFT_BLACK);
      for (int i = 0; i < 4; ++i) M5.Display.drawFastHLine(x-r+13, y-r/3+i*(r/2), size-26, TFT_BLACK);
      M5.Display.drawFastHLine(x-r+13, y-r+3, r, TFT_BLACK); break;
    case Icon::FolderAdd:
      M5.Display.fillRoundRect(x-r,y-r/2,size,r+8,3,TFT_BLACK); M5.Display.fillRoundRect(x-r+3,y-r+2,r,9,3,TFT_BLACK);
      M5.Display.fillRect(x+3,y-6,14,4,TFT_WHITE); M5.Display.fillRect(x+8,y-11,4,14,TFT_WHITE); break;
    case Icon::Select:
      M5.Display.drawRoundRect(x-r+3,y-r+3,size-6,size-6,3,TFT_BLACK);
      M5.Display.drawLine(x-r/3,y+2,x-3,y+r/3,TFT_BLACK); M5.Display.drawLine(x-3,y+r/3,x+r-4,y-r/3,TFT_BLACK);
      M5.Display.drawLine(x-r/3,y+4,x-3,y+r/3+2,TFT_BLACK); M5.Display.drawLine(x-3,y+r/3+2,x+r-4,y-r/3+2,TFT_BLACK); break;
    case Icon::Delete:
      M5.Display.fillRoundRect(x-r+5,y-r+1,size-10,size-5,3,TFT_BLACK); M5.Display.fillRect(x-r+2,y-r-4,size-4,4,TFT_BLACK);
      M5.Display.fillRect(x-6,y-r-8,12,4,TFT_BLACK); M5.Display.drawFastVLine(x-6,y-r+6,size-16,TFT_WHITE); M5.Display.drawFastVLine(x+6,y-r+6,size-16,TFT_WHITE); break;
    case Icon::Move:
      M5.Display.fillRect(x-r,y-3,size-10,6,TFT_BLACK); M5.Display.fillTriangle(x+r,y,x+r-10,y-10,x+r-10,y+10,TFT_BLACK);
      M5.Display.fillRect(x-r+3,y-12,6,24,TFT_BLACK); M5.Display.fillTriangle(x-r+6,y-r,x-r-4,y-r+10,x-r+16,y-r+10,TFT_BLACK); break;
    case Icon::Refresh:
      M5.Display.drawArc(x,y, r-5,r,35,300,TFT_BLACK); M5.Display.fillTriangle(x+r-2,y-r/3,x+r-13,y-r/3-4,x+r-9,y+8,TFT_BLACK); break;
    case Icon::Close:
      // A true 45-degree close mark, drawn three pixels thick so it remains
      // crisp on e-paper rather than looking like a plus sign.
      M5.Display.drawLine(x-r+4,y-r+4,x+r-4,y+r-4,TFT_BLACK);
      M5.Display.drawLine(x-r+5,y-r+4,x+r-3,y+r-4,TFT_BLACK);
      M5.Display.drawLine(x-r+3,y-r+4,x+r-5,y+r-4,TFT_BLACK);
      M5.Display.drawLine(x+r-4,y-r+4,x-r+4,y+r-4,TFT_BLACK);
      M5.Display.drawLine(x+r-5,y-r+4,x-r+3,y+r-4,TFT_BLACK);
      M5.Display.drawLine(x+r-3,y-r+4,x-r+5,y+r-4,TFT_BLACK); break;
    case Icon::Check:
      M5.Display.drawLine(x-r+3,y,x-4,y+r-5,TFT_BLACK); M5.Display.drawLine(x-4,y+r-5,x+r-2,y-r+4,TFT_BLACK);
      M5.Display.drawLine(x-r+3,y+2,x-4,y+r-3,TFT_BLACK); M5.Display.drawLine(x-4,y+r-3,x+r-2,y-r+6,TFT_BLACK); break;
    case Icon::Todo:
      // Quill To Do: a simple circular completion mark and tick.
      M5.Display.drawCircle(x, y, r-3, TFT_BLACK);
      M5.Display.drawCircle(x, y, r-4, TFT_BLACK);
      M5.Display.drawLine(x-r/3, y, x-2, y+r/3, TFT_BLACK);
      M5.Display.drawLine(x-2, y+r/3, x+r/3+4, y-r/3, TFT_BLACK); break;
    case Icon::Power:
      // Compact adaptation of fa-solid--power-off. It uses a filled broken
      // ring and a rounded vertical stem, retaining the source icon's solid
      // silhouette at the requested 18px footer size.
      M5.Display.drawArc(x, y + 1, r - 3, r - 1, 38, 322, TFT_BLACK);
      M5.Display.drawArc(x, y + 1, r - 4, r - 2, 38, 322, TFT_BLACK);
      M5.Display.fillRoundRect(x - (size >= 20 ? 3 : 2), y - r, size >= 20 ? 6 : 4, r + 6, 2, TFT_BLACK); break;
    case Icon::System:
      M5.Display.fillRoundRect(x-r,y-r,size,size-5,3,TFT_BLACK); M5.Display.fillRect(x-r+4,y-r+4,size-8,size-13,TFT_WHITE); M5.Display.fillRect(x-r+8,y-r+9,6,6,TFT_BLACK); M5.Display.fillRect(x+2,y-r+9,6,6,TFT_BLACK); M5.Display.fillRect(x-r+8,y-1,6,6,TFT_BLACK); M5.Display.fillRect(x+2,y-1,6,6,TFT_BLACK); M5.Display.fillRect(x-r/2,y+r-1,r,4,TFT_BLACK); break;
    case Icon::Calculator:
      M5.Display.fillRoundRect(x-r,y-r,size,size,4,TFT_BLACK);
      M5.Display.fillRoundRect(x-r+5,y-r+5,size-10,r/2,2,TFT_WHITE);
      for (int row=0; row<3; ++row) for (int col=0; col<3; ++col)
        M5.Display.fillCircle(x-r/2+col*(r/2), y-1+row*(r/2-1), 3, TFT_WHITE);
      M5.Display.fillCircle(x+r/2, y-1, 3, TFT_WHITE); M5.Display.fillCircle(x+r/2, y+r/2-1, 3, TFT_WHITE); break;
    case Icon::Settings:
      // Codicon Settings: two compact slider controls, clearer than a gear in the footer.
      M5.Display.fillRoundRect(x-r+2, y-r/3-4, size-4, 3, 2, TFT_BLACK);
      M5.Display.fillRoundRect(x-r+2, y+r/3+2, size-4, 3, 2, TFT_BLACK);
      M5.Display.fillCircle(x-r/4, y-r/3-3, 5, TFT_BLACK);
      M5.Display.fillCircle(x+r/4, y+r/3+3, 5, TFT_BLACK); break;
    case Icon::Wifi:
      M5.Display.drawArc(x,y+7,r-5,r,225,315,TFT_BLACK); M5.Display.drawArc(x,y+7,r/2-2,r/2+3,225,315,TFT_BLACK); M5.Display.fillCircle(x,y+12,4,TFT_BLACK); break;
    case Icon::Battery:
      M5.Display.fillRoundRect(x-r,y-r/2,size-5,r,3,TFT_BLACK); M5.Display.fillRect(x+r-5,y-5,5,10,TFT_BLACK); M5.Display.fillRect(x-r+3,y-r/2+3,size-12,r-6,TFT_WHITE); M5.Display.fillRect(x-r+3,y-r/2+3,size-17,r-6,TFT_BLACK); break;
    case Icon::Sleep: M5.Display.fillCircle(x,y,r-3,TFT_BLACK); M5.Display.fillCircle(x+r/3,y-r/3,r-3,TFT_WHITE); break;
    case Icon::Play: M5.Display.fillTriangle(x-r/3,y-r+3,x-r/3,y+r-3,x+r-2,y,TFT_BLACK); break;
    case Icon::Pause: M5.Display.fillRect(x-r/2,y-r+2,6,size-4,TFT_BLACK); M5.Display.fillRect(x+r/2-6,y-r+2,6,size-4,TFT_BLACK); break;
    case Icon::Stop: M5.Display.fillRoundRect(x-r+3,y-r+3,size-6,size-6,3,TFT_BLACK); break;
    case Icon::Shift:
      M5.Display.fillTriangle(x,y-r,x-r+2,y+2,x+r-2,y+2,TFT_BLACK);
      M5.Display.fillRect(x-5,y,10,r,TFT_BLACK); M5.Display.fillRect(x-10,y+r-3,20,4,TFT_BLACK); break;
    case Icon::Backspace:
      M5.Display.fillTriangle(x-r,y,x-r+9,y-r+8,x-r+9,y+r-8,TFT_BLACK);
      M5.Display.fillRoundRect(x-r+7,y-r+8,size-12,size-16,3,TFT_BLACK);
      M5.Display.drawLine(x-2,y-5,x+7,y+5,TFT_WHITE); M5.Display.drawLine(x+7,y-5,x-2,y+5,TFT_WHITE); break;
    case Icon::ArrowRight:
      // A compact, solid chevron keeps the folder affordance clear at a
      // glance without competing with file rows that cannot be opened.
      M5.Display.fillTriangle(x-r/3,y-r+2,x-r/3,y+r-2,x+r/2,y,TFT_BLACK); break;
    case Icon::Pen:
      // Solar pen-bold: chunky diagonal body, nib, and small end cap.
      for (int i = -3; i <= 3; ++i) M5.Display.drawLine(x - r / 2 + i, y + r / 2, x + r / 2 + i, y - r / 2, TFT_BLACK);
      M5.Display.fillTriangle(x - r / 2 - 3, y + r / 2 + 4, x - r / 2 + 7, y + r / 2, x - r / 2, y + r / 2 - 7, TFT_BLACK);
      M5.Display.fillCircle(x + r / 2 - 1, y - r / 2 + 1, 4, TFT_BLACK); break;
    case Icon::Undo:
      // Rivet undo: a solid left arrow flowing into a returning stroke.
      M5.Display.fillTriangle(x-r,y,x-r/3,y-r/2,x-r/3,y+r/2,TFT_BLACK);
      M5.Display.fillRect(x-r/3,y-3,r+4,6,TFT_BLACK);
      M5.Display.drawArc(x+r/4,y+7,r/2-2,r/2+2,205,355,TFT_BLACK);
      M5.Display.drawArc(x+r/4,y+7,r/2-3,r/2+1,205,355,TFT_BLACK); break;
    case Icon::Eraser:
      // Griddy filled eraser: diagonal block plus its characteristic baseline.
      M5.Display.fillTriangle(x-r,y+4,x-2,y-r,x+r,y+4,TFT_BLACK);
      M5.Display.fillTriangle(x-r,y+4,x+r,y+4,x+2,y+r-4,TFT_BLACK);
      M5.Display.fillRect(x-r/3,y+r-2,r+r/2,4,TFT_BLACK); break;
  }
}

}  // namespace ui
