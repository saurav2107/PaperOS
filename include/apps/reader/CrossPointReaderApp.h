#pragma once
#include "app/IApp.h"
#include "apps/native/StatusApp.h"
#include "apps/reader/crosspoint/CrossPointEpubAdapter.h"

class CrossPointReaderApp final : public IApp {
 public:
  void setNavigator(NavigationCallback navigator) { navigator_ = navigator; }
  AppId id() const override { return AppId::Reader; }
  const char* title() const override { return "eReader"; }
  bool onStart(AppContext&) override;
  void onTick(AppContext&, uint32_t) override;
  void onStop(AppContext&) override {}
 private:
  enum class BookFormat : uint8_t { Epub, Text, Markdown, PdfPages };
  struct BookEntry { String path; String title; BookFormat format; uint16_t pages; uint32_t bytes; bool opened; uint32_t resumeOffset; };

  static constexpr uint8_t PageSize = 5;
  static constexpr uint8_t MaxBooks = 32;
  static constexpr uint8_t MaxTextPages = 96;
  struct EpubPosition { uint16_t block; uint16_t character; };
  void scanLibrary(AppContext&);
  void draw(AppContext&);
  void drawLibrary(AppContext&);
  void drawReader(AppContext&);
  void drawReaderContent();
  void drawReaderFooter();
  void drawTextPage();
  void drawEpubPage();
  void drawPdfPage();
  void drawAppearance(AppContext&);
  void drawChapters(AppContext&);
  void loadAppearance();
  void saveAppearance() const;
  void selectReaderBodyFont() const;
  void resetEpubPagination();
  uint8_t epubProgressPercent() const;
  bool jumpToEpubPercent(uint8_t percent, String& error);
  void openSelected(AppContext&);
  static bool loadPdfManifest(const String& folder, String& title, uint16_t& pages);
  static String displayName(const String& path);
  static const char* formatLabel(BookFormat format);
  static bool loadProgress(const String& path, uint32_t& offset);
  static void saveProgress(const String& path, uint32_t offset);
  static bool loadEpubProgress(const String& path, uint16_t& chapter, EpubPosition& position);
  static void saveEpubProgress(const String& path, uint16_t chapter, const EpubPosition& position);
  NavigationCallback navigator_{nullptr};
  BookEntry books_[MaxBooks];
  uint8_t bookCount_{0};
  uint8_t selected_{0};
  uint8_t page_{0};
  bool reading_{false};
  bool readingPdfPages_{false};
  bool readingEpub_{false};
  bool appearanceOpen_{false};
  bool chapterPickerOpen_{false};
  uint8_t chapterPickerPage_{0};
  // These are persisted separately from system UI fonts: a reading font must
  // reflow books, while chrome remains stable and predictable.
  uint8_t readerFont_{0};       // 0 Serif, 1 Sans
  uint8_t readerScale_{1};      // 1 normal, 2 large
  uint8_t readerSpacing_{1};    // 0 compact, 1 normal, 2 relaxed
  uint16_t epubChapter_{0};
  CrossPointEpubAdapter epub_;
  EpubPosition epubOffsets_[MaxTextPages]{};
  EpubPosition epubPageEnd_{};
  uint8_t textPage_{0};
  uint16_t pdfPage_{1};
  uint32_t textOffsets_[MaxTextPages]{};
  uint32_t textPageEnd_{0};
  String status_;
};
