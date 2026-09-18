# CrossPoint Reader integration

## Upstream source

- Repository: `https://github.com/juicecultus/crosspoint-reader-papers3`
- Pinned commit: `d9792a58b415f37e6f9de688241467db85048d05`
- License: MIT

## Current OS-native reader

The active `CrossPointReaderApp` owns Paper OS integration only:

- scans EPUB, TXT, Markdown, and converted-PDF folders in `/PaperOS/books`;
- reserves `/.crosspoint` for CrossPoint's SD cache/progress model;
- opens TXT and Markdown books directly with paged `PREV`/`NEXT` navigation;
- opens converted PDFs as one pre-rendered PNG page at a time; see
  `docs/PDF_READER.md` and `tools/pdf_to_papers3.py`;
- uses touch-to-open and Paper OS `HOME`/`PREV`/`NEXT` chrome (no Confirm action);
- does not initialize display, SD, Wi-Fi, OTA, or deep sleep.

## EPUB adapter: phase 1

`CrossPointEpubAdapter` is now the first active port slice. It keeps all
hardware ownership in Paper OS and reads an EPUB directly from SD by:

1. locating `META-INF/container.xml` in the ZIP archive;
2. parsing `content.opf` for the manifest and reading spine;
3. inflating each XHTML spine item with the ESP32-S3 ROM inflater;
4. preserving paragraphs, headings, quotations, lists, dividers, and image
   positions as semantic blocks; and
5. laying those blocks out with book-style typography through the native
   `PREV`/`NEXT` reader.

The first slice supports ordinary EPUB 2/3 text chapters with a conservative
48 KB per-archive-entry limit. It deliberately
separates EPUB parsing from M5Unified drawing so CrossPoint's CSS/page-cache
engine can be introduced without changing Paper OS navigation. It does not
yet render embedded images, CSS styling, tables, footnotes, DRM, or encrypted
EPUBs; an image placeholder keeps page flow stable until the image decoder is
ported.

## Next migration phases

1. Port CrossPoint's CSS-aware text layout and bounded chapter cache.
2. Persist spine/page progress under `/.crosspoint`.
3. Add embedded image rendering and cover extraction.
4. Add table, footnote, and chapter-navigation support without importing
   CrossPoint's Wi-Fi, web server, OTA, sleep, or global activity manager.

## Adding another format

Keep file detection in `CrossPointReaderApp::scanLibrary`, then add a dedicated
adapter rather than passing bytes directly to the display:

1. Add a `BookFormat` value and extension check.
2. Create a reader that yields one laid-out page at a time from SD/PSRAM.
3. Reuse the existing `HOME`/`PREV`/`NEXT` navigation and offset/progress model.
4. Keep large decode buffers bounded and release them in `onStop`.

TXT/Markdown are native text readers. EPUB needs ZIP + XML/HTML + layout;
PDF needs a PDF rasterizer; comic archives need ZIP/CBZ image decoding. Those
are separate adapters with different memory budgets, not variants of the TXT
reader.

Do not compile upstream `src/main.cpp` alongside Paper OS.
