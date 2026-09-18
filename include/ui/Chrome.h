#pragma once
#include <Arduino.h>

struct AppContext;

namespace ui {

enum class FooterAction : uint8_t { None, Back, Up, Previous, PowerOff, Home, Down, Confirm, Close, Next, Settings };

struct ChromeOptions {
  bool showBack{true};
  bool showUp{false};
  bool showPrevious{false};
  bool showPowerOff{false};
  bool showHome{true};
  bool showDown{false};
  bool showConfirm{false};
  bool showClose{false};
  bool showNext{false};
  bool showSettings{false};
};

// Shared, minimal e-paper chrome. Apps own their content area only.
class Chrome {
 public:
  // Runtime metrics keep navigation usable if the display rotation changes.
  static int headerHeight();
  static int footerHeight();
  static int footerTop();
  static void drawHeader(AppContext& context, const char* title, bool legacyShowBack = false, bool uppercaseTitle = true);
  static void drawFooter();
  static void drawFooter(const ChromeOptions& options);
  static FooterAction hitTestFooter(int x, int y);
  static FooterAction hitTestFooter(int x, int y, const ChromeOptions& options);
  // Maps a horizontal content swipe to an exposed PREV/NEXT action. Apps keep
  // ownership of the resulting page change, while chrome keeps gesture rules
  // consistent.
  static FooterAction swipeAction(int startX, int startY, int endX, int endY, const ChromeOptions& options);
};

}  // namespace ui
