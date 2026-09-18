#pragma once

#include <Arduino.h>

// First CrossPoint adapter slice.  It deliberately owns only EPUB parsing
// state; Paper OS continues to own display, SD mounting, navigation, and
// power.  The implementation follows CrossPoint's container/OPF/spine model.
class CrossPointEpubAdapter {
 public:
  // EPUB content is exposed as semantic blocks rather than a single stripped
  // string.  The reader can therefore give headings, paragraphs, quotations,
  // and lists their own typography and spacing while the next port phase adds
  // CSS properties and inline spans.
  enum class BlockKind : uint8_t { Paragraph, Heading1, Heading2, Heading3, Quote, ListItem, Image, Divider };
  // Alignment and indentation are intentionally renderer-neutral.  The next
  // CrossPoint CSS import can enrich this model without coupling it to M5GFX.
  struct Block { BlockKind kind; String text; String resource; uint8_t alignment{0}; uint8_t indent{0}; };
  struct TocEntry { String title; uint16_t chapter; };

  bool open(const String& path, String& error);
  void close();
  bool loadChapter(uint16_t chapter, String& error);

  bool ready() const { return ready_; }
  uint16_t chapterCount() const { return chapterCount_; }
  uint16_t initialChapter() const { return initialChapter_; }
  const String& title() const { return title_; }
  const String& chapterText() const { return chapterText_; }
  uint16_t blockCount() const { return blockCount_; }
  const Block& block(uint16_t index) const { return blocks_[index]; }
  uint8_t tocCount() const { return tocCount_; }
  const TocEntry& tocEntry(uint8_t index) const { return toc_[index]; }
  // Materialises one embedded JPG/PNG to the SD cache. Rendering remains the
  // app's responsibility, keeping ZIP/PSRAM ownership out of the UI layer.
  bool materializeImage(uint16_t chapter, const String& resource, String& cachedPath, String& error) const;

 private:
  struct ZipEntry { uint16_t method; uint32_t compressed; uint32_t uncompressed; uint32_t localOffset; };
  struct ManifestItem { String id; String href; String media; };

  static constexpr uint8_t MaxManifestItems = 64;
  static constexpr uint8_t MaxChapters = 48;
  bool readEntry(const String& name, String& content, String& error) const;
  bool extractEntryToFile(const String& name, const String& target, String& error) const;
  bool findEntry(const String& name, ZipEntry& entry) const;
  bool loadCachedChapter(uint16_t chapter);
  void saveCachedChapter(uint16_t chapter) const;
  String cacheFile(uint16_t chapter) const;
  static String attribute(const String& tag, const char* name);
  static String normalisePath(const String& base, const String& relative);
  void parseBlocks(const String& markup);
  void appendBlock(BlockKind kind, String text, const String& resource = "", uint8_t alignment = 0, uint8_t indent = 0);

  String path_;
  String cssText_;
  uint32_t sourceBytes_{0};
  String title_;
  String chapterPaths_[MaxChapters];
  static constexpr uint8_t MaxTocEntries = 48;
  TocEntry toc_[MaxTocEntries];
  uint8_t tocCount_{0};
  String chapterText_;
  static constexpr uint16_t MaxBlocks = 192;
  Block blocks_[MaxBlocks];
  uint16_t blockCount_{0};
  uint16_t chapterCount_{0};
  uint16_t initialChapter_{0};
  bool ready_{false};
};
