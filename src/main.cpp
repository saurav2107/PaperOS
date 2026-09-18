#include <Arduino.h>
#include <M5Unified.h>
#include "app/AppManager.h"
#include "apps/launcher/LauncherApp.h"
#include "apps/clock/ClockApp.h"
#include "apps/pomodoro/PomodoroApp.h"
#include "apps/weather/WeatherApp.h"
#include "apps/dashboard/DashboardApp.h"
#include "apps/reader/CrossPointReaderApp.h"
#include "apps/notes/NotesApp.h"
#include "apps/images/ImagesApp.h"
#include "apps/files/FileBrowserApp.h"
#include "apps/flashcards/FlashcardsApp.h"
#include "apps/chess/ChessApp.h"
#include "apps/tetris/TetrisApp.h"
#include "apps/sudoku/SudokuApp.h"
#include "apps/snake/SnakeApp.h"
#include "apps/minesweeper/MinesweeperApp.h"
#include "apps/settings/SettingsApp.h"
#include "apps/todo/TodoApp.h"
#include "apps/calendar/CalendarApp.h"
#include "apps/native/StatusApp.h"
#include "apps/utilities/UtilitiesApps.h"
#include "services/Services.h"

namespace {
Services services;
AppManager manager{services.context()};
LauncherApp launcher;
ClockApp clockApp;
CalendarApp calendarApp;
ImagesApp imageViewerApp;
CrossPointReaderApp epubApp;
DashboardApp dashboardApp;
WeatherApp weatherApp;
PomodoroApp pomodoroApp;
SettingsApp settingsApp;
NotesApp notesApp;
ChessApp chessApp;
TetrisApp tetrisApp;
SudokuApp sudokuApp;
SnakeApp snakeApp;
MinesweeperApp minesweeperApp;
FileBrowserApp filesApp;
FlashcardsApp flashcardsApp;
TodoApp todoApp;
CalculatorApp calculatorApp;
ConverterApp converterApp;
AlarmApp alarmApp;
ShoppingListApp shoppingListApp;
BackupRestoreApp backupRestoreApp;
WritingApp writingApp;
SystemMonitorApp systemMonitorApp;

void navigateTo(AppId app) { manager.activate(app); }

bool preventsInactivitySleep(AppId app) {
  // These screens have an active purpose: Pomodoro is a running timer, Clock
  // is intended as an always-on desk display, and Weather is allowed to wait
  // for its configured background refresh while visible.
  return app == AppId::Pomodoro || app == AppId::Clock || app == AppId::Weather;
}
}

void setup() {
  Serial.begin(115200);
  auto config = M5.config();
  M5.begin(config);

  // PaperS3 has a passive buzzer on GPIO21. M5.begin configures it but does
  // not start the speaker worker; start it once as a shared OS capability.
  auto speakerConfig = M5.Speaker.config();
  speakerConfig.pin_data_out = GPIO_NUM_21;
  speakerConfig.buzzer = true;
  speakerConfig.magnification = 48;
  M5.Speaker.config(speakerConfig);
  M5.Speaker.begin();

  if (!psramFound()) {
    M5.Display.println("PSRAM not detected. Check board memory type: qio_opi.");
    while (true) delay(1000);
  }

  services.begin();
  manager.registerApp(launcher);
  manager.registerApp(clockApp);
  manager.registerApp(calendarApp);
  manager.registerApp(imageViewerApp);
  manager.registerApp(epubApp);
  manager.registerApp(dashboardApp);
  manager.registerApp(weatherApp);
  manager.registerApp(pomodoroApp);
  manager.registerApp(settingsApp);
  manager.registerApp(notesApp); manager.registerApp(todoApp); manager.registerApp(chessApp); manager.registerApp(tetrisApp); manager.registerApp(sudokuApp); manager.registerApp(snakeApp); manager.registerApp(minesweeperApp);
  manager.registerApp(filesApp); manager.registerApp(flashcardsApp);
  manager.registerApp(calculatorApp); manager.registerApp(converterApp); manager.registerApp(alarmApp);
  manager.registerApp(backupRestoreApp); manager.registerApp(writingApp); manager.registerApp(systemMonitorApp);
  launcher.setNavigator(navigateTo);
  settingsApp.setNavigator(navigateTo);
  clockApp.setNavigator(navigateTo); calendarApp.setNavigator(navigateTo); todoApp.setNavigator(navigateTo);
  imageViewerApp.setNavigator(navigateTo); epubApp.setNavigator(navigateTo);
  dashboardApp.setNavigator(navigateTo); weatherApp.setNavigator(navigateTo);
  pomodoroApp.setNavigator(navigateTo); notesApp.setNavigator(navigateTo);
  chessApp.setNavigator(navigateTo); tetrisApp.setNavigator(navigateTo); sudokuApp.setNavigator(navigateTo); snakeApp.setNavigator(navigateTo); minesweeperApp.setNavigator(navigateTo); filesApp.setNavigator(navigateTo); flashcardsApp.setNavigator(navigateTo);
  calculatorApp.setNavigator(navigateTo); converterApp.setNavigator(navigateTo); alarmApp.setNavigator(navigateTo); backupRestoreApp.setNavigator(navigateTo); writingApp.setNavigator(navigateTo); systemMonitorApp.setNavigator(navigateTo);
  manager.activate(AppId::Launcher);
}

void loop() {
  services.setSleepInhibited(preventsInactivitySleep(manager.activeId()));
  services.tick();
  // After touch wakes the shared light-sleep screen, return to a known safe
  // surface instead of redrawing a stale, resource-heavy application.
  if (services.consumeWakeEvent()) manager.activate(AppId::Launcher);
  manager.tick();
  delay(5);
}
