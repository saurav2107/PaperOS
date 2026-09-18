#pragma once

#include <Arduino.h>

namespace ui {

// Compact Material-style glyphs drawn directly on the PaperS3. Keeping this
// subset in firmware is clearer and substantially smaller than embedding an
// entire Material Symbols font for a handful of application icons.
enum class Icon : uint8_t {
  Home, Weather, Clock, Calendar, Book, Image, Timer, Note, Sketch,
  Games, HomeAssistant, Folder, Flashcards, FolderAdd, Select, Delete, Move, Refresh, Close, Check, Todo, Power,
  System, Calculator, Settings, Wifi, Battery, Sleep, Play, Pause, Stop, Shift, Backspace, ArrowRight, Pen, Undo, Eraser
};

void drawIcon(Icon icon, int centerX, int centerY, int size = 44);

}  // namespace ui
