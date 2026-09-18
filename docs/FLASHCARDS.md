# Flashcards: rich dinosaur cards

The offline Dinosaur deck is stored on the SD card:

```text
/PaperOS/flashcards/dinosaurs.json
/PaperOS/flashcards/images/
```

The first launch creates `dinosaurs.json` with the starter 200 cards. Existing
cards that only contain `name` and `fact` remain valid. Add optional image and
fact fields to make any card richer:

```json
{
  "name": "Tyrannosaurus rex",
  "fact": "A large tyrannosaur from Late Cretaceous North America. Its deep skull and strong bite made it one of the best-known predators of its time.",
  "era": "Late Cretaceous",
  "diet": "Carnivore",
  "length": "Up to 12 m",
  "region": "North America",
  "image": "images/tyrannosaurus-rex.jpg"
}
```

Use the exact JSON structure below for the deck file:

```json
{
  "deck": "DINOSAURS",
  "cards": [
    { "name": "...", "fact": "...", "image": "images/example.jpg" }
  ]
}
```

Image rules:

- Put images under `/PaperOS/flashcards/images`.
- Use JPG/JPEG or PNG only.
- Export images at **180 × 130 pixels** (or smaller) in portrait orientation.
- Use a relative filename such as `images/triceratops.png`; never use `..` or
  an absolute path.
- Keep each image modest in size (preferably below 100 KB) to preserve PSRAM
  and keep e-paper rendering fast.

The **RANDOM** button selects a different card each time, while PREV/NEXT keep
the normal deck order for study. The application also starts on a random card.

## Optional Gemini card generation

1. On PaperS3, open **Settings → Flashcard AI → Enable AP Setup**.
2. Join `PaperOS-Setup` and open `http://192.168.4.1`.
3. Enter a Gemini API key, text model, and—if your Gemini plan supports it—an
   image model. Save settings.
4. In Flashcards, use **CATEGORY** to choose Dinosaurs, Space, Animals,
   Science, Geography, or History, then tap **GENERATE**.

The generated card is appended to that category's SD-card deck immediately.
When Gemini returns an image no larger than the PaperS3 safety limit, it is
stored under `/PaperOS/flashcards/images` and linked to the new card. A card is
still saved when image generation is unsupported, unavailable, or quota-limited.

HTTP `429` is reported as an AI limit message; PaperS3 does not automatically
retry and consume further quota. Gemini API model availability, free quotas,
and image-generation eligibility are controlled by the user's Google project.
