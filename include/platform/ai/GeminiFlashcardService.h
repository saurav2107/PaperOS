#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

class DeviceSettingsService;

struct GeneratedFlashcard {
  bool success{false};
  bool imageSaved{false};
  char name[80]{}, fact[520]{}, era[48]{}, diet[48]{}, length[48]{}, region[80]{}, image[96]{}, message[112]{};
};

// A bounded worker for opt-in Gemini card generation. Network, JSON and base64
// work stay outside loopTask; the app only consumes a fixed-size result.
class GeminiFlashcardService {
 public:
  bool request(const String& category, const DeviceSettingsService& settings);
  bool inProgress() const { return inProgress_; }
  bool consume(GeneratedFlashcard& result);
 private:
  static void worker(void* argument);
  void run(const String& category, const String& key, const String& textModel, const String& imageModel);
  portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
  TaskHandle_t task_{nullptr};
  volatile bool inProgress_{false};
  bool ready_{false};
  GeneratedFlashcard result_{};
};
