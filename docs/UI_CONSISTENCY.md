# Paper OS UI Consistency Rules

Keep every application visually and behaviorally consistent with the PaperS3
OS shell. Treat `ui::Chrome` as the shared layout contract, not as optional
decoration.

## Navigation

- Use `ui::Chrome::drawHeader()` and `ui::Chrome::drawFooter()` in every app.
- Prefer the shared HOME action for returning to the launcher. Hide BACK when
  HOME is sufficient.
- Use PREV/NEXT only for an app's own page, date, item, or media navigation.
- Do not draw over `Chrome::headerHeight()` or below `Chrome::footerTop()`.

## Buttons and icons

- Use a solid black outer frame with a white 3px inset for tappable buttons.
- Keep button labels uppercase, bold, centred, and large enough to read on
  e-paper. Pair an icon with any action where it improves recognition.
- Use the shared `ui::Icon` set before introducing a new hand-drawn glyph.
- Keep repeated controls the same height and width with even gaps. Do not use
  single-pixel outline-only buttons for primary actions.
- Reserve the top-right corner for compact page-level actions such as refresh.

## E-paper updates

- Never redraw the whole screen for a small state change.
- Keyboard presses redraw only the text field; timers redraw only their timer
  region; games redraw only changed game pixels where practical.
- Use a full redraw only when entering an app, changing layout/style, changing
  orientation, or explicitly pressing Refresh.

## Layout and typography

- Use runtime Chrome metrics instead of fixed header/footer coordinates.
- Leave visible gaps between panels, text baselines, borders, and icons.
- Clip or ellipsize long labels before they cross a border.
- Avoid unsupported UTF-8 glyphs in embedded fonts. Use the app-wide safe
  temperature unit format (` C` / ` F`) rather than a broken degree glyph.
- Preserve portrait/landscape ownership: an app that sets landscape must
  restore portrait in `onStop()`.

## Validation checklist

Before shipping an app UI change, verify that every button is fully visible,
has a matching touch target, uses a consistent frame/icon/label style, and
updates only the smallest required display region.
