#include "apps/reader/crosspoint/CrossPointEpubAdapter.h"

#include <SD.h>
#include <esp32/rom/miniz.h>
#include <esp_heap_caps.h>

namespace {
uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t le32(const uint8_t* p) { return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24); }
// The adapter keeps one decompressed XHTML/OPF item plus a String copy.  A
// conservative limit avoids a fragmented heap or watchdog reset on a malformed
// EPUB; normal reflowable book chapters are far below this size.
constexpr size_t kMaxEntryBytes = 48 * 1024;
uint32_t cacheKey(const String& value) {
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < value.length(); ++i) { hash ^= static_cast<uint8_t>(value[i]); hash *= 16777619UL; }
  return hash;
}
bool ensureCacheSpace(size_t bytesNeeded) {
  constexpr uint64_t kReserveBytes = 512UL * 1024UL;
  if (SD.totalBytes() >= SD.usedBytes() && SD.totalBytes() - SD.usedBytes() >= bytesNeeded + kReserveBytes) return true;
  File directory = SD.open("/PaperOS/cache");
  if (directory && directory.isDirectory()) {
    for (File entry = directory.openNextFile(); entry; entry = directory.openNextFile()) {
      const String name = entry.name(); const bool isDirectory = entry.isDirectory(); entry.close();
      if (!isDirectory) SD.remove(name);
    }
  }
  if (directory) directory.close();
  return SD.totalBytes() >= SD.usedBytes() && SD.totalBytes() - SD.usedBytes() >= bytesNeeded + kReserveBytes;
}
}

void CrossPointEpubAdapter::close() {
  path_ = title_ = chapterText_ = cssText_ = "";
  sourceBytes_ = 0;
  chapterCount_ = blockCount_ = initialChapter_ = tocCount_ = 0;
  ready_ = false;
}

String CrossPointEpubAdapter::attribute(const String& tag, const char* name) {
  const String key = String(name) + "=";
  const int start = tag.indexOf(key);
  if (start < 0) return "";
  const int quoteAt = start + key.length();
  if (quoteAt >= static_cast<int>(tag.length())) return "";
  const char quote = tag[quoteAt];
  const int end = tag.indexOf(quote, quoteAt + 1);
  return end < 0 ? "" : tag.substring(quoteAt + 1, end);
}

String CrossPointEpubAdapter::normalisePath(const String& base, const String& relative) {
  String combined = base + relative;
  while (combined.indexOf("../") >= 0) {
    const int parent = combined.indexOf("../");
    const int slash = combined.lastIndexOf('/', parent - 2);
    if (slash < 0) break;
    combined.remove(slash + 1, parent + 3 - slash - 1);
  }
  while (combined.indexOf("./") >= 0) combined.replace("./", "");
  return combined;
}

bool CrossPointEpubAdapter::findEntry(const String& name, ZipEntry& entry) const {
  File file = SD.open(path_, FILE_READ);
  if (!file) return false;
  const size_t size = file.size();
  if (size < 22) { file.close(); return false; }
  const size_t tailSize = size < 65557 ? size : 65557;
  uint8_t* tail = static_cast<uint8_t*>(ps_malloc(tailSize));
  if (!tail) { file.close(); return false; }
  file.seek(size - tailSize); file.read(tail, tailSize);
  int eocd = -1;
  for (int i = static_cast<int>(tailSize) - 22; i >= 0; --i) {
    if (le32(tail + i) == 0x06054b50) { eocd = i; break; }
  }
  if (eocd < 0) { free(tail); file.close(); return false; }
  const uint16_t entries = le16(tail + eocd + 10);
  uint32_t position = le32(tail + eocd + 16);
  free(tail);
  if (position >= size) { file.close(); return false; }
  uint8_t header[46];
  for (uint16_t i = 0; i < entries; ++i) {
    if (!file.seek(position) || file.read(header, sizeof(header)) != sizeof(header) || le32(header) != 0x02014b50) break;
    const uint16_t nameLength = le16(header + 28), extraLength = le16(header + 30), commentLength = le16(header + 32);
    if (nameLength > 255 || position + 46UL + nameLength + extraLength + commentLength > size) break;
    String candidate;
    candidate.reserve(nameLength);
    for (uint16_t c = 0; c < nameLength; ++c) candidate += static_cast<char>(file.read());
    if (candidate == name) {
      entry = {le16(header + 10), le32(header + 20), le32(header + 24), le32(header + 42)};
      file.close(); return true;
    }
    position += 46UL + nameLength + extraLength + commentLength;
  }
  file.close(); return false;
}

bool CrossPointEpubAdapter::readEntry(const String& name, String& content, String& error) const {
  ZipEntry entry{};
  if (!findEntry(name, entry)) { error = "EPUB ENTRY NOT FOUND: " + name; return false; }
  if (!entry.uncompressed || entry.uncompressed > kMaxEntryBytes || entry.compressed > kMaxEntryBytes) { error = "EPUB SECTION IS TOO LARGE"; return false; }
  // Decompression needs compressed input, output, and a temporary String copy.
  // Reject rather than exhausting heap/PSRAM and resetting the whole OS.
  const size_t requiredPsram = entry.compressed + entry.uncompressed + 16384;
  const size_t requiredHeap = entry.uncompressed * 2 + 16384;
  if (!psramFound() || ESP.getFreePsram() < requiredPsram || ESP.getFreeHeap() < requiredHeap) { error = "NOT ENOUGH MEMORY FOR EPUB CHAPTER"; return false; }
  Serial.printf("[EPUB] reading %s (%lu -> %lu bytes)\n", name.c_str(), static_cast<unsigned long>(entry.compressed), static_cast<unsigned long>(entry.uncompressed));
  File file = SD.open(path_, FILE_READ);
  uint8_t local[30];
  if (!file || !file.seek(entry.localOffset) || file.read(local, sizeof(local)) != sizeof(local) || le32(local) != 0x04034b50) { if(file)file.close(); error="INVALID EPUB ZIP HEADER"; return false; }
  const uint32_t dataOffset = entry.localOffset + 30UL + le16(local + 26) + le16(local + 28);
  uint8_t* compressed = static_cast<uint8_t*>(ps_malloc(entry.compressed));
  uint8_t* output = static_cast<uint8_t*>(ps_malloc(entry.uncompressed + 1));
  if (!compressed || !output || !file.seek(dataOffset) || file.read(compressed, entry.compressed) != entry.compressed) { if(compressed)free(compressed); if(output)free(output); file.close(); error="NOT ENOUGH MEMORY FOR EPUB SECTION"; return false; }
  file.close();
  bool valid = false;
  if (entry.method == 0) { memcpy(output, compressed, entry.uncompressed); valid = true; }
  else if (entry.method == 8) {
    // tinfl_decompress_mem_to_mem() creates its large Huffman tables on the
    // caller's stack. loopTask has a deliberately modest stack, so allocate
    // the inflater in PSRAM and invoke the low-level routine directly.
    // ROM code is safest with its state in internal 8-bit RAM. This remains
    // off loopTask's stack, which is what caused the prior canary reset.
    tinfl_decompressor* inflater = static_cast<tinfl_decompressor*>(heap_caps_malloc(sizeof(tinfl_decompressor), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (inflater) {
      tinfl_init(inflater);
      size_t inputSize = entry.compressed, outputSize = entry.uncompressed;
      const tinfl_status status = tinfl_decompress(inflater, compressed, &inputSize, output, output, &outputSize,
                                                    TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
      Serial.printf("[EPUB] inflate status=%d input=%u output=%u\n", static_cast<int>(status), static_cast<unsigned>(inputSize), static_cast<unsigned>(outputSize));
      valid = status == TINFL_STATUS_DONE && outputSize == entry.uncompressed;
      free(inflater);
    }
  }
  free(compressed);
  if (!valid) { free(output); error = "EPUB DECOMPRESSION FAILED"; return false; }
  output[entry.uncompressed] = 0;
  content = reinterpret_cast<char*>(output);
  free(output);
  Serial.printf("[EPUB] inflated %s\n", name.c_str());
  return true;
}

bool CrossPointEpubAdapter::extractEntryToFile(const String& name, const String& target, String& error) const {
  ZipEntry entry{};
  if (!findEntry(name, entry)) { error = "EPUB IMAGE NOT FOUND"; return false; }
  // Images are streamed into the SD cache after one bounded decode buffer.
  // The limit protects the shared firmware heap from malformed publications.
  constexpr size_t kMaxImageBytes = 180 * 1024;
  if (!entry.uncompressed || entry.uncompressed > kMaxImageBytes || entry.compressed > kMaxImageBytes) { error = "EPUB IMAGE IS TOO LARGE"; return false; }
  if (!ensureCacheSpace(entry.uncompressed)) { error = "NOT ENOUGH SD SPACE FOR EPUB IMAGE"; return false; }
  File source = SD.open(path_, FILE_READ); uint8_t local[30];
  if (!source || !source.seek(entry.localOffset) || source.read(local, sizeof(local)) != sizeof(local) || le32(local) != 0x04034b50) { if(source)source.close(); error = "INVALID EPUB IMAGE"; return false; }
  const uint32_t dataOffset = entry.localOffset + 30UL + le16(local + 26) + le16(local + 28);
  uint8_t* input = static_cast<uint8_t*>(ps_malloc(entry.compressed));
  uint8_t* output = static_cast<uint8_t*>(ps_malloc(entry.uncompressed));
  if (!input || !output || !source.seek(dataOffset) || source.read(input, entry.compressed) != entry.compressed) { if(input)free(input); if(output)free(output); source.close(); error = "NOT ENOUGH MEMORY FOR IMAGE"; return false; }
  source.close(); bool valid = false;
  if (entry.method == 0) { memcpy(output, input, entry.uncompressed); valid = true; }
  else if (entry.method == 8) {
    tinfl_decompressor* inflater = static_cast<tinfl_decompressor*>(heap_caps_malloc(sizeof(tinfl_decompressor), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (inflater) { tinfl_init(inflater); size_t inSize = entry.compressed, outSize = entry.uncompressed;
      valid = tinfl_decompress(inflater, input, &inSize, output, output, &outSize, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF) == TINFL_STATUS_DONE && outSize == entry.uncompressed; free(inflater); }
  }
  free(input); if (!valid) { free(output); error = "EPUB IMAGE DECOMPRESSION FAILED"; return false; }
  const String temporary = target + ".part";
  if (SD.exists(temporary)) SD.remove(temporary);
  File destination = SD.open(temporary, FILE_WRITE);
  const bool written = destination && destination.write(output, entry.uncompressed) == entry.uncompressed;
  if (destination) destination.close(); free(output);
  if (!written) { if (SD.exists(temporary)) SD.remove(temporary); error = "COULD NOT CACHE EPUB IMAGE"; return false; }
  if (SD.exists(target)) SD.remove(target);
  if (!SD.rename(temporary, target)) { if (SD.exists(temporary)) SD.remove(temporary); error = "COULD NOT FINALIZE EPUB IMAGE CACHE"; return false; }
  return true;
}

bool CrossPointEpubAdapter::materializeImage(uint16_t chapter, const String& resource, String& cachedPath, String& error) const {
  if (chapter >= chapterCount_ || resource.isEmpty()) { error = "EPUB IMAGE IS UNAVAILABLE"; return false; }
  const int slash = chapterPaths_[chapter].lastIndexOf('/');
  const String base = slash < 0 ? "" : chapterPaths_[chapter].substring(0, slash + 1);
  const String entry = normalisePath(base, resource);
  String extension = entry.substring(entry.lastIndexOf('.')); extension.toLowerCase();
  if (extension != ".png" && extension != ".jpg" && extension != ".jpeg") { error = "UNSUPPORTED EPUB IMAGE"; return false; }
  SD.mkdir("/PaperOS"); SD.mkdir("/PaperOS/cache");
  cachedPath = String("/PaperOS/cache/img_") + String(cacheKey(path_), HEX) + "_" + chapter + "_" + String(cacheKey(entry), HEX) + extension;
  if (SD.exists(cachedPath)) return true;
  return extractEntryToFile(entry, cachedPath, error);
}

String CrossPointEpubAdapter::cacheFile(uint16_t chapter) const {
  char value[68];
  snprintf(value, sizeof(value), "/PaperOS/cache/epub_%08lx_chapter_%u.cache",
           static_cast<unsigned long>(cacheKey(path_)), static_cast<unsigned>(chapter));
  return String(value);
}

bool CrossPointEpubAdapter::loadCachedChapter(uint16_t chapter) {
  File file = SD.open(cacheFile(chapter), FILE_READ);
  if (!file) return false;
  const String expected = String("P3EPUB3|") + sourceBytes_;
  String header = file.readStringUntil('\n'); header.trim();
  if (header != expected) { file.close(); return false; }
  blockCount_ = 0; chapterText_ = "";
  while (file.available() && blockCount_ < MaxBlocks) {
    String line = file.readStringUntil('\n');
    const int first = line.indexOf('\t');
    const int second = first < 0 ? -1 : line.indexOf('\t', first + 1);
    if (first < 1 || second < 0) { file.close(); blockCount_ = 0; chapterText_ = ""; return false; }
    const int kindValue = line.substring(0, first).toInt();
    if (kindValue < static_cast<int>(BlockKind::Paragraph) || kindValue > static_cast<int>(BlockKind::Divider)) continue;
    const String resource = line.substring(first + 1, second);
    const String text = line.substring(second + 1);
    Block& block = blocks_[blockCount_++];
    block.kind = static_cast<BlockKind>(kindValue); block.text = text; block.resource = resource;
    block.alignment = 0; block.indent = 0;
    if (!text.isEmpty() && kindValue != static_cast<int>(BlockKind::Image)) {
      if (!chapterText_.isEmpty()) chapterText_ += '\n';
      chapterText_ += text;
    }
  }
  file.close();
  if (!blockCount_ || chapterText_.isEmpty()) { blockCount_ = 0; chapterText_ = ""; return false; }
  Serial.printf("[EPUB] loaded cached chapter %u\n", static_cast<unsigned>(chapter));
  return true;
}

void CrossPointEpubAdapter::saveCachedChapter(uint16_t chapter) const {
  if (!blockCount_ || !sourceBytes_) return;
  SD.mkdir("/PaperOS"); SD.mkdir("/PaperOS/cache");
  if (!ensureCacheSpace(48UL * 1024UL)) return;
  const String path = cacheFile(chapter), temporary = path + ".part";
  if (SD.exists(temporary)) SD.remove(temporary);
  File file = SD.open(temporary, FILE_WRITE);
  if (!file) return;
  file.println(String("P3EPUB3|") + sourceBytes_);
  for (uint16_t i = 0; i < blockCount_; ++i) {
    file.print(static_cast<uint8_t>(blocks_[i].kind)); file.print('\t');
    file.print(blocks_[i].resource); file.print('\t'); file.println(blocks_[i].text);
  }
  file.close();
  if (SD.exists(path)) SD.remove(path);
  if (!SD.rename(temporary, path) && SD.exists(temporary)) SD.remove(temporary);
}

void CrossPointEpubAdapter::appendBlock(BlockKind kind, String text, const String& resource, uint8_t alignment, uint8_t indent) {
  text.trim();
  while (text.indexOf("  ") >= 0) text.replace("  ", " ");
  if (kind != BlockKind::Image && kind != BlockKind::Divider && text.isEmpty()) return;
  if (blockCount_ >= MaxBlocks) return;
  Block& block = blocks_[blockCount_++];
  block.kind = kind; block.text = text; block.resource = resource;
  block.alignment = alignment; block.indent = indent;
  // An <img> node has a structural position but no readable text.  Keeping
  // its "IMAGE" placeholder out of chapterText prevents cover-only spine
  // entries from being mistaken for a readable chapter.
  if (!text.isEmpty() && kind != BlockKind::Image) {
    if (!chapterText_.isEmpty()) chapterText_ += '\n';
    chapterText_ += text;
  }
}

void CrossPointEpubAdapter::parseBlocks(const String& markup) {
  blockCount_ = 0;
  chapterText_ = "";
  BlockKind current = BlockKind::Paragraph;
  uint8_t currentAlignment = 0, currentIndent = 0;
  bool currentHidden = false;
  String text;
  bool inTag = false;
  bool ignoredContent = false;
  String tag;
  const auto flush = [&]() { if (!currentHidden) appendBlock(current, text, "", currentAlignment, currentIndent); text = ""; };
  const auto decodeEntity = [](const String& entity) -> String {
    if (entity == "amp") return "&";
    if (entity == "lt") return "<";
    if (entity == "gt") return ">";
    if (entity == "quot") return "\"";
    if (entity == "apos") return "'";
    if (entity == "nbsp") return " ";
    // A full Unicode entity table arrives with the upstream CrossPoint parser.
    // Preserve a readable placeholder until then instead of silently joining words.
    return " ";
  };
  const auto cssDeclarations = [this](const String& element, const String& classes) {
    String result;
    int cursor = 0;
    while (cursor < static_cast<int>(cssText_.length())) {
      const int open = cssText_.indexOf('{', cursor); if (open < 0) break;
      const int close = cssText_.indexOf('}', open + 1); if (close < 0) break;
      String selector = cssText_.substring(cursor, open); selector.toLowerCase();
      bool matches = selector.indexOf(element) >= 0;
      int classCursor = 0;
      while (!matches && classCursor < static_cast<int>(classes.length())) {
        const int space = classes.indexOf(' ', classCursor);
        String name = classes.substring(classCursor, space < 0 ? classes.length() : space); name.toLowerCase();
        if (!name.isEmpty() && selector.indexOf("." + name) >= 0) matches = true;
        classCursor = space < 0 ? classes.length() : space + 1;
      }
      if (matches) { result += cssText_.substring(open + 1, close); result += ';'; }
      cursor = close + 1;
    }
    result.toLowerCase(); return result;
  };
  for (size_t i = 0; i < markup.length(); ++i) {
    const char c = markup[i];
    if (inTag) {
      if (c != '>') { tag += c; continue; }
      inTag = false;
      tag.trim();
      String lower = tag; lower.toLowerCase();
      const bool closing = lower.startsWith("/");
      if (closing) lower.remove(0, 1);
      const int space = lower.indexOf(' ');
      const String name = space >= 0 ? lower.substring(0, space) : lower;
      const bool blockTag = name == "p" || name == "div" || name == "section" || name == "article" ||
                            name == "blockquote" || name == "li" || name == "h1" || name == "h2" || name == "h3" ||
                            name == "h4" || name == "h5" || name == "h6";
      if (name == "style" || name == "script" || name == "head" || name == "nav") {
        // CSS, JavaScript, document metadata, and navigation documents are
        // part of an EPUB package but are not reader-visible book content.
        if (closing) ignoredContent = false; else { flush(); ignoredContent = true; }
      } else if (ignoredContent) {
        // Ignore nested markup until the matching outer non-reading section
        // closes. EPUB style sheets commonly contain visible-looking strings
        // such as ".cover-image { max-width: 100% }".
      } else if (name == "br") { text += '\n'; }
      else if (name == "hr") { flush(); appendBlock(BlockKind::Divider, ""); }
      else if (name == "img" && !closing) {
        flush();
        const String source = attribute(tag, "src");
        appendBlock(BlockKind::Image, "IMAGE", source);
      } else if (blockTag) {
        if (closing) { flush(); current = BlockKind::Paragraph; currentAlignment = currentIndent = 0; currentHidden = false; }
        else {
          flush();
          const String style = attribute(tag, "style");
          String styleLower = cssDeclarations(name, attribute(tag, "class"));
          styleLower += ';'; styleLower += style; styleLower.toLowerCase();
          currentAlignment = styleLower.indexOf("text-align:center") >= 0 || styleLower.indexOf("text-align: center") >= 0 ? 1
            : styleLower.indexOf("text-align:right") >= 0 || styleLower.indexOf("text-align: right") >= 0 ? 2 : 0;
          currentIndent = styleLower.indexOf("text-indent") >= 0 ? 1 : 0;
          currentHidden = styleLower.indexOf("display:none") >= 0 || styleLower.indexOf("display: none") >= 0;
          if (name == "h1") current = BlockKind::Heading1;
          else if (name == "h2") current = BlockKind::Heading2;
          else if (name == "h3" || name == "h4" || name == "h5" || name == "h6") current = BlockKind::Heading3;
          else if (name == "blockquote") current = BlockKind::Quote;
          else if (name == "li") current = BlockKind::ListItem;
          else current = BlockKind::Paragraph;
        }
      }
      tag = "";
      continue;
    }
    if (c == '<') { inTag = true; tag = ""; continue; }
    if (ignoredContent) continue;
    if (c == '&') {
      const int end = markup.indexOf(';', i + 1);
      if (end > static_cast<int>(i) && end - static_cast<int>(i) < 16) {
        text += decodeEntity(markup.substring(i + 1, end));
        i = end;
        continue;
      }
    }
    if (c == '\r' || c == '\n' || c == '\t') text += ' '; else text += c;
  }
  flush();
  if (!blockCount_ && !markup.isEmpty()) appendBlock(BlockKind::Paragraph, "UNSUPPORTED EPUB CHAPTER CONTENT");
}

bool CrossPointEpubAdapter::open(const String& path, String& error) {
  close(); path_ = path;
  File source = SD.open(path_, FILE_READ);
  if (!source) { error = "COULD NOT OPEN EPUB FILE"; return false; }
  sourceBytes_ = source.size(); source.close();
  Serial.printf("[EPUB] opening %s\n", path_.c_str());
  String container;
  if (!readEntry("META-INF/container.xml", container, error)) return false;
  Serial.println("[EPUB] container ready");
  // Do not match the enclosing <rootfiles> node: only <rootfile ...> carries
  // the full-path attribute for the OPF package document.
  const int root = container.indexOf("<rootfile ");
  if (root < 0) { error = "INVALID EPUB CONTAINER"; return false; }
  const int rootEnd = container.indexOf('>', root);
  const String opfPath = attribute(container.substring(root, rootEnd + 1), "full-path");
  if (opfPath.isEmpty()) { error = "EPUB CONTENT FILE IS MISSING"; return false; }
  String opf;
  if (!readEntry(opfPath, opf, error)) return false;
  Serial.printf("[EPUB] OPF ready: %s\n", opfPath.c_str());
  const int titleStart = opf.indexOf("<dc:title");
  if (titleStart >= 0) { const int textStart = opf.indexOf('>', titleStart); const int textEnd = opf.indexOf('<', textStart + 1); if(textStart>=0&&textEnd>textStart) title_ = opf.substring(textStart + 1, textEnd); }
  if (title_.isEmpty()) title_ = path.substring(path.lastIndexOf('/') + 1);
  // Keep this cache out of loopTask's stack. Each item contains three Arduino
  // Strings, so a 64-entry local array can overflow the FreeRTOS task stack.
  static ManifestItem manifest[MaxManifestItems]; uint8_t manifestCount = 0;
  int cursor = 0;
  while (manifestCount < MaxManifestItems && (cursor = opf.indexOf("<item ", cursor)) >= 0) {
    const int end = opf.indexOf('>', cursor); if(end < 0) break;
    const String tag = opf.substring(cursor, end + 1);
    manifest[manifestCount++] = {attribute(tag,"id"), attribute(tag,"href"), attribute(tag,"media-type")}; cursor = end + 1;
  }
  const int baseEnd = opfPath.lastIndexOf('/'); const String base = baseEnd >= 0 ? opfPath.substring(0, baseEnd + 1) : "";
  // Load small external stylesheets once.  The full CrossPoint CSS engine
  // will replace this compact selector subset, but keeping CSS here means
  // parsing stays independent of the renderer and cached page model.
  for (uint8_t i = 0; i < manifestCount; ++i) {
    if (manifest[i].media.indexOf("css") < 0) continue;
    String stylesheet, cssError;
    if (readEntry(normalisePath(base, manifest[i].href), stylesheet, cssError) && stylesheet.length() <= 48 * 1024) {
      cssText_ += stylesheet; cssText_ += '\n';
    }
  }
  cursor = 0;
  while (chapterCount_ < MaxChapters && (cursor = opf.indexOf("<itemref", cursor)) >= 0) {
    const int end = opf.indexOf('>', cursor); if(end < 0) break; const String id = attribute(opf.substring(cursor, end + 1), "idref");
    for (uint8_t i = 0; i < manifestCount; ++i) if (manifest[i].id == id && (manifest[i].media.indexOf("html") >= 0 || manifest[i].media.indexOf("xhtml") >= 0)) { chapterPaths_[chapterCount_++] = normalisePath(base, manifest[i].href); break; }
    cursor = end + 1;
  }
  if (!chapterCount_) { error = "NO EPUB TEXT CHAPTERS FOUND"; close(); return false; }
  // Fallback TOC is available for every EPUB from its ordered spine. The next
  // parser slice replaces these file-derived labels with EPUB 2 NCX / EPUB 3
  // NAV document titles where the publication provides them.
  for (uint16_t chapter = 0; chapter < chapterCount_ && tocCount_ < MaxTocEntries; ++chapter) {
    String label = chapterPaths_[chapter];
    const int slash = label.lastIndexOf('/'); if (slash >= 0) label = label.substring(slash + 1);
    const int dot = label.lastIndexOf('.'); if (dot > 0) label = label.substring(0, dot);
    label.replace('_', ' '); label.replace('-', ' ');
    if (label.isEmpty()) label = String("Chapter ") + (chapter + 1);
    TocEntry& entry = toc_[tocCount_++]; entry.title = label; entry.chapter = chapter;
  }
  // Prefer authored EPUB 2 NCX labels when available; retain the spine list
  // as a safe fallback for books with incomplete navigation metadata.
  for (uint8_t item = 0; item < manifestCount; ++item) {
    if (manifest[item].media.indexOf("ncx") < 0) continue;
    String ncx, navError;
    if (!readEntry(normalisePath(base, manifest[item].href), ncx, navError)) continue;
    int cursor = 0;
    while ((cursor = ncx.indexOf("<navPoint", cursor)) >= 0) {
      const int end = ncx.indexOf("</navPoint>", cursor); if (end < 0) break;
      const String point = ncx.substring(cursor, end);
      const int textOpen = point.indexOf("<text>"); const int textClose = point.indexOf("</text>");
      const int srcAt = point.indexOf("src=");
      if (textOpen >= 0 && textClose > textOpen && srcAt >= 0) {
        const int quote = srcAt + 4; const int srcEnd = point.indexOf(point[quote], quote + 1);
        String target = srcEnd > quote ? point.substring(quote + 1, srcEnd) : "";
        const int fragment = target.indexOf('#'); if (fragment >= 0) target = target.substring(0, fragment);
        target = normalisePath(base, target);
        for (uint8_t chapter = 0; chapter < tocCount_; ++chapter) if (chapterPaths_[chapter] == target) {
          toc_[chapter].title = point.substring(textOpen + 6, textClose); toc_[chapter].title.trim(); break;
        }
      }
      cursor = end + 11;
    }
    break;
  }
  Serial.printf("[EPUB] parsed %u spine chapters\n", chapterCount_);
  ready_ = true;
  // EPUB spine item zero is frequently a cover page.  Until image rendering
  // lands, choose the first chapter with actual readable text rather than
  // opening a blank IMAGE placeholder before the book starts.
  String lastError;
  for (uint16_t chapter = 0; chapter < chapterCount_; ++chapter) {
    if (loadChapter(chapter, lastError)) { initialChapter_ = chapter; return true; }
  }
  error = lastError.isEmpty() ? "NO EPUB TEXT CHAPTERS FOUND" : lastError;
  close();
  return false;
}

bool CrossPointEpubAdapter::loadChapter(uint16_t chapter, String& error) {
  if (!ready_ || chapter >= chapterCount_) { error = "EPUB CHAPTER IS UNAVAILABLE"; return false; }
  if (loadCachedChapter(chapter)) return true;
  String markup;
  if (!readEntry(chapterPaths_[chapter], markup, error)) return false;
  parseBlocks(markup);
  if (!blockCount_ || chapterText_.isEmpty()) { error = "EPUB CHAPTER HAS NO READABLE TEXT"; return false; }
  saveCachedChapter(chapter);
  return true;
}
