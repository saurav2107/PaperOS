# Chess integration

`ChessApp` adapts the board layout, PNG piece convention, and human-versus-device
turn flow from [`arunmathaisk/PaperS3-chess`](https://github.com/arunmathaisk/PaperS3-chess).
It is kept as an app module rather than running the upstream standalone `setup()`
and `loop()`, because Paper OS owns M5Unified, touch, display, and microSD once.

## SD-card assets

Copy all twelve PNG files from
`assets/chess/` to this exact microSD directory (license retained alongside the assets):

```text
/PaperOS/chess/white_pawn.png
/PaperOS/chess/white_rook.png
... (all remaining PNG files)
```

The app falls back to letter pieces if the images or SD card are unavailable, so
the launcher and game can still be tested. The assets remain on SD rather than
firmware flash or PSRAM: they are decoded only while a square is drawn.

## Scope of this first module

It implements normal geometry, captures, double pawn advances, promotion to a
queen, and a low-cost single-ply Black response. King-safety validation and
checkmate/stalemate detection for both sides are implemented. Castling,
en passant, repetition and fifty-move draws remain unimplemented. The unused
upstream checkout has been removed; provenance is in `docs/THIRD_PARTY.md`.
