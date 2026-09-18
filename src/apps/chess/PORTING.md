# Chess integration

`ChessApp` adapts the board layout, PNG piece convention, and human-versus-device
turn flow from [`arunmathaisk/PaperS3-chess`](https://github.com/arunmathaisk/PaperS3-chess).
It is kept as an app module rather than running the upstream standalone `setup()`
and `loop()`, because Paper OS owns M5Unified, touch, display, and microSD once.

## SD-card assets

Copy all twelve PNG files from
`third_party/papers3-chess/assets/chess_pieces/` to this exact microSD directory:

```text
/chess/white_pawn.png
/chess/white_rook.png
... (all remaining PNG files)
```

The app falls back to letter pieces if the images or SD card are unavailable, so
the launcher and game can still be tested. The assets remain on SD rather than
firmware flash or PSRAM: they are decoded only while a square is drawn.

## Scope of this first module

It implements normal geometry, captures, double pawn advances, promotion to a
queen, and a low-cost single-ply Black response. Advanced rules from upstream
(check validation, castling, en passant, checkmate search) are deliberately the
next engine extraction step; they belong in a separate `ChessEngine` class, not
in the UI or lifecycle code.
