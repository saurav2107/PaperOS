#!/usr/bin/env python3
"""Convert a PDF into a Paper OS SD-card book folder.

Requires Poppler's ``pdftoppm`` and Pillow:
  brew install poppler
  python3 -m pip install Pillow

Example:
  python3 tools/pdf_to_papers3.py ~/Downloads/guide.pdf --output /Volumes/NO_NAME/PaperOS/books
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageOps

PANEL_WIDTH = 540
CONTENT_HEIGHT = 780


def page_image(source: Path, destination: Path) -> None:
    """Resize one rendered PDF page into the eReader's content canvas."""
    with Image.open(source) as input_image:
        image = ImageOps.exif_transpose(input_image).convert("L")
        resampling = getattr(Image, "Resampling", Image).LANCZOS
        image.thumbnail((PANEL_WIDTH, CONTENT_HEIGHT), resampling)
        canvas = Image.new("L", (PANEL_WIDTH, CONTENT_HEIGHT), 255)
        x = (PANEL_WIDTH - image.width) // 2
        y = (CONTENT_HEIGHT - image.height) // 2
        canvas.paste(image, (x, y))
        # Four neutral levels retain text edges while remaining e-ink friendly.
        canvas = canvas.point(lambda value: min(255, (value // 64) * 85))
        canvas.save(destination, "PNG", optimize=True)


def main() -> None:
    parser = argparse.ArgumentParser(description="Create a Paper OS PDF page-image book")
    parser.add_argument("pdf", type=Path, help="Source PDF file")
    parser.add_argument("--output", type=Path, required=True, help="Destination /PaperOS/books directory on the SD card")
    parser.add_argument("--title", help="Book title shown in the Library (defaults to PDF filename)")
    parser.add_argument("--dpi", type=int, default=150, help="Poppler render DPI before resize (default: 150)")
    args = parser.parse_args()

    if not args.pdf.is_file():
        parser.error(f"PDF not found: {args.pdf}")
    if not shutil.which("pdftoppm"):
        parser.error("pdftoppm is required. On macOS install it with: brew install poppler")

    book_title = args.title or args.pdf.stem
    book_folder = args.output / args.pdf.stem
    book_folder.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="papers3-pdf-") as temporary:
        prefix = Path(temporary) / "rendered"
        subprocess.run(
            ["pdftoppm", "-png", "-r", str(args.dpi), str(args.pdf), str(prefix)], check=True
        )
        source_pages = sorted(
            Path(temporary).glob("rendered-*.png"),
            key=lambda page: int(page.stem.rsplit("-", 1)[1]),
        )
        if not source_pages:
            raise RuntimeError("Poppler did not produce any page images")
        for number, source_page in enumerate(source_pages, start=1):
            page_image(source_page, book_folder / f"page_{number:03d}.png")

    (book_folder / "manifest.txt").write_text(
        f"PAPERS3_PDF=1\nTITLE={book_title}\nPAGES={len(source_pages)}\n", encoding="utf-8"
    )
    print(f"Created {len(source_pages)} PaperS3 pages in: {book_folder}")
    print("Safely eject the SD card, open eReader > Library, and tap the new book.")


if __name__ == "__main__":
    main()
