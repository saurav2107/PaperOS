#pragma once

#include "app/IApp.h"
#include "apps/native/StatusApp.h"
#include "ui/Chrome.h"

// Offline-first flashcards. The Dinosaur deck lives on the SD card at
// /PaperOS/flashcards/dinosaurs.json, so it can be replaced or extended without a
// firmware upload.
class FlashcardsApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Flashcards; }
  const char* title() const override { return "Flashcards"; }
  bool onStart(AppContext& context) override;
  void onTick(AppContext& context, uint32_t nowMs) override;
  void onStop(AppContext&) override {}

 private:
  struct Card {
    String name;
    String fact;
    String image;   // Relative path below /PaperOS/flashcards, JPG or PNG.
    String era;
    String diet;
    String length;
    String region;
  };
  static constexpr uint16_t MaxCards = 200;

  bool ensureStarterDeck(AppContext& context);
  bool loadDeck(AppContext& context);
  bool saveDeck(AppContext& context);
  void draw(AppContext& context);
  void drawCategoryLanding(AppContext& context);
  void drawCardContent(AppContext& context);
  void drawActionBar(AppContext& context);
  void drawStatus();
  void refreshCardContent(AppContext& context);
  void showCard(AppContext& context, uint16_t index);
  void showRandomCard(AppContext& context);
  bool drawCardImage(const String& relativePath, int x, int y) const;
  bool cardImageAvailable(const String& relativePath) const;
  String deckPath() const;
  const char* categoryName() const;
  const char* categoryLabel() const;
  ui::ChromeOptions chromeOptions() const;

  Card cards_[MaxCards];
  uint16_t cardCount_{0};
  uint16_t current_{0};
  uint8_t category_{0};
  bool categoryLanding_{true};
  String status_;
  NavigationCallback navigator_{nullptr};
};
