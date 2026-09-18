#pragma once
#include <Arduino.h>

struct AppContext;

enum class AppId : uint8_t {
  Launcher, Clock, Calendar, Images, Reader, Dashboard, Weather, Pomodoro, Settings,
  Notes, Tasks, Music, Habits, Calculator, Converter, Files, Flashcards, Rss, Sensors, Terminal, SystemMonitor,
  Alarm, ShoppingList, BackupRestore, Writing, Chess, Tetris, Sudoku, Snake, Minesweeper
};

class IApp {
 public:
  virtual ~IApp() = default;
  virtual AppId id() const = 0;
  virtual const char* title() const = 0;
  virtual bool onStart(AppContext& context) = 0;
  virtual void onTick(AppContext& context, uint32_t nowMs) = 0;
  virtual void onStop(AppContext& context) = 0;
};
