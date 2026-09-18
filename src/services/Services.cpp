#include "services/Services.h"
#include <M5Unified.h>
#include "ui/Typography.h"

// Composition root: this is the only place native capabilities are assembled.
Services::Services()
    : context_{display_, settings_, input_, uiSound_, storage_, network_, time_, power_, weatherSettings_, temperature_, clockFace_, pomodoroStyle_, weatherFace_, weatherFetch_, geminiFlashcards_, tasks_} {}

void Services::begin() {
  settings_.begin();
  display_.begin();
  display_.setFlipped(settings_.displayFlipped());
  network_.setSettings(&settings_);
  uiSound_.setSettings(&settings_);
  time_.setSettings(&settings_);
  power_.setSettings(&settings_);
  weatherSettings_.setSettings(&settings_);
  temperature_.setSettings(&settings_);
  ui::Typography::begin();
  uiSound_.begin();
  storage_.begin();
  network_.begin();
  time_.begin();
  power_.begin();
  weatherSettings_.begin();
  temperature_.begin();
  clockFace_.begin();
  pomodoroStyle_.begin();
  weatherFace_.begin();
}

void Services::tick() {
  input_.update();
  if (M5.Touch.getDetail().wasPressed()) { power_.noteActivity(); uiSound_.click(); }
  network_.tick();
  time_.tick();
  power_.tick(time_.formattedLocalTime());
}
