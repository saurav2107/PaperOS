# PDF page-image reader

Paper OS reads PDFs as pre-rendered, e-ink-friendly page images. This avoids
putting a full PDF rasterizer and its large temporary buffers on the ESP32-S3.
The original PDF layout, tables, diagrams, and fonts are preserved.

## One-time Mac setup

```sh
brew install poppler
python3 -m pip install Pillow
```

## One-time Windows setup

Open PowerShell and install Poppler plus Pillow:

```powershell
winget install oschwartz10612.Poppler
py -m pip install Pillow
```

## Convert a book for the SD card

With the SD card mounted, run this from the Paper OS project folder. Replace
`NO_NAME` with the actual SD-card volume name.

```sh
python3 tools/pdf_to_papers3.py ~/Downloads/book.pdf --output /Volumes/NO_NAME/PaperOS/books
```

On Windows, replace `E:` with the drive letter assigned to the microSD card:

```powershell
.\tools\Convert-PdfToPaperS3.ps1 -Pdf "$HOME\Downloads\book.pdf" -Output "E:\PaperOS\books"
```

The converter creates:

```text
/PaperOS/books/book/
  manifest.txt
  page_001.png
  page_002.png
  ...
```

The Library recognises a folder only when `manifest.txt` contains
`PAPERS3_PDF=1`, a `TITLE`, and a `PAGES` count. Do not rename the numbered
page files.

Each page is rendered as a 540 x 780 PNG, which fits between Paper OS's shared
header and footer. The PDF reader keeps only the current PNG in the normal
display decoding path; it does not load the entire document into RAM.

Use `PREV` and `NEXT` to turn pages and `HOME` to leave the reader.
