#include "apps/flashcards/FlashcardsApp.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <esp_system.h>
#include <M5Unified.h>

#include "app/AppContext.h"
#include "services/Services.h"
#include "ui/Chrome.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

namespace {
constexpr const char* kDeckFolder = "/PaperOS/flashcards";
constexpr const char* kImageFolder = "/PaperOS/flashcards/images";
constexpr const char* kDeckPath = "/PaperOS/flashcards/dinosaurs.json";
constexpr const char* kCategories[] = {"DINOSAURS", "SPACE", "ANIMALS", "SCIENCE", "GEOGRAPHY", "HISTORY"};
constexpr const char* kCategoryLabels[] = {"Dinosaurs", "Space", "Animals", "Science", "Geography", "History"};
constexpr const char* kCategoryFiles[] = {"dinosaurs.json", "space.json", "animals.json", "science.json", "geography.json", "history.json"};
constexpr uint8_t kCategoryCount = sizeof(kCategories) / sizeof(kCategories[0]);

// 200 starter cards. The summary is intentionally broad by dinosaur group;
// users can enrich individual cards directly in the SD-card JSON file.
const char* const kDinosaurNames[] = {
  "Tyrannosaurus","Tarbosaurus","Albertosaurus","Gorgosaurus","Daspletosaurus","Alioramus","Qianzhousaurus","Bistahieversor","Lythronax","Teratophoneus",
  "Dilong","Eotyrannus","Guanlong","Proceratosaurus","Yutyrannus","Dryptosaurus","Appalachiosaurus","Moros","Suskityrannus","Coelurus",
  "Allosaurus","Saurophaganax","Acrocanthosaurus","Carcharodontosaurus","Giganotosaurus","Mapusaurus","Neovenator","Megalosaurus","Torvosaurus","Ceratosaurus",
  "Carnotaurus","Majungasaurus","Abelisaurus","Rugops","Noasaurus","Masiakasaurus","Velociraptor","Deinonychus","Utahraptor","Dromaeosaurus",
  "Microraptor","Sinornithosaurus","Bambiraptor","Saurornitholestes","Stenonychosaurus","Archaeopteryx","Compsognathus","Ornithomimus","Struthiomimus","Gallimimus",
  "Deinocheirus","Oviraptor","Citipati","Gigantoraptor","Therizinosaurus","Segnosaurus","Nothronychus","Falcarius","Mononykus","Anzu",
  "Apatosaurus","Brontosaurus","Diplodocus","Brachiosaurus","Camarasaurus","Barosaurus","Supersaurus","Argentinosaurus","Patagotitan","Dreadnoughtus",
  "Futalognkosaurus","Saltasaurus","Alamosaurus","Mamenchisaurus","Shunosaurus","Omeisaurus","Euhelopus","Jobaria","Giraffatitan","Europasaurus",
  "Plateosaurus","Massospondylus","Riojasaurus","Mussaurus","Lufengosaurus","Cetiosaurus","Vulcanodon","Isanosaurus","Spinophorosaurus","Atlasaurus",
  "Rebbachisaurus","Nigersaurus","Limaysaurus","Dicraeosaurus","Amargasaurus","Bajadasaurus","Rapetosaurus","Aegyptosaurus","Sonorasaurus","Cedarosaurus",
  "Iguanodon","Hadrosaurus","Edmontosaurus","Parasaurolophus","Corythosaurus","Lambeosaurus","Maiasaura","Gryposaurus","Saurolophus","Brachylophosaurus",
  "Ouranosaurus","Mantellisaurus","Dryosaurus","Camptosaurus","Tenontosaurus","Hypsilophodon","Leaellynasaura","Thescelosaurus","Parksosaurus","Orodromeus",
  "Zephyrosaurus","Heterodontosaurus","Lesothosaurus","Gasparinisaura","Muttaburrasaurus","Equijubus","Shantungosaurus","Magnapaulia","Tethyshadros","Jeholosaurus",
  "Triceratops","Styracosaurus","Centrosaurus","Pachyrhinosaurus","Chasmosaurus","Pentaceratops","Torosaurus","Protoceratops","Psittacosaurus","Leptoceratops",
  "Zuniceratops","Diabloceratops","Einiosaurus","Avaceratops","Nasutoceratops","Regaliceratops","Kosmoceratops","Vagaceratops","Xenoceratops","Wendiceratops",
  "Agujaceratops","Utahceratops","Sinoceratops","Turanoceratops","Bagaceratops","Montanoceratops","Udanoceratops","Yinlong","Chaoyangsaurus","Archaeoceratops",
  "Stegosaurus","Kentrosaurus","Huayangosaurus","Gigantspinosaurus","Miragaia","Dacentrurus","Chungkingosaurus","Tuojiangosaurus","Ankylosaurus","Euoplocephalus",
  "Edmontonia","Panoplosaurus","Sauropelta","Borealopelta","Nodosaurus","Polacanthus","Gastonia","Minmi","Pinacosaurus","Saichania",
  "Pachycephalosaurus","Stegoceras","Prenocephale","Homalocephale","Dracorex","Stygimoloch","Yi","Epidexipteryx","Kulindadromeus","Chilesaurus",
  "Eoraptor","Herrerasaurus","Staurikosaurus","Eodromaeus","Coelophysis","Thecodontosaurus","Anchisaurus","Saturnalia","Panphagia","Pampadromaeus"
};

constexpr uint16_t kStarterCardCount = sizeof(kDinosaurNames) / sizeof(kDinosaurNames[0]);

const char* groupFact(uint16_t index) {
  if (index < 60) return "A theropod dinosaur. Theropods were generally two-legged dinosaurs, and many were meat-eaters.";
  if (index < 100) return "A sauropodomorph dinosaur. This group includes long-necked plant-eaters and their early relatives.";
  if (index < 130) return "An ornithischian plant-eater. Many members of this broad group had beaks for cropping vegetation.";
  if (index < 160) return "A ceratopsian dinosaur. Ceratopsians are known for beaks, frills, and in many species, facial horns.";
  if (index < 180) return "An armored dinosaur. Armor plates, spikes, or bony scutes helped protect many of these plant-eaters.";
  return "A dinosaur from the diverse Mesozoic world. Use this card as a starting point and add your own studied notes.";
}

void frame(int x, int y, int width, int height, int radius = 12) {
  ui::Theme::drawFrame(x, y, width, height, radius);
}

String trimText(const String& text, int maxWidth) {
  if (M5.Display.textWidth(text) <= maxWidth) return text;
  String result = text;
  while (!result.isEmpty() && M5.Display.textWidth(result + "...") > maxWidth) result.remove(result.length() - 1);
  return result + "...";
}
void actionButton(int x, int y, int width, int height, const char* label) {
  ui::Theme::drawButtonFrame(x, y, width, height);
  // Four compact controls share this row. Match Paper OS's standard action
  // button typography so labels neither dominate nor touch the frame edges.
  M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString(label, x + width / 2, y + height / 2 + 1);
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
}
void tableCell(int x, int y, int width, int height, const char* label, const String& value) {
  ui::Theme::drawFrame(x, y, width, height, 0);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b);M5.Display.setTextDatum(TL_DATUM);M5.Display.drawString(label,x+12,y+12);
  M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString(trimText(value,width-22),x+width/2,y+height-23);
  M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}
int drawWrappedText(const String& text, int centerX, int y, int width, int maxLines, int lineHeight) {
  String remaining=text, line; int count=0;
  while(!remaining.isEmpty() && count<maxLines) {
    const int split=remaining.indexOf(' '); const String word=split<0?remaining:remaining.substring(0,split);
    const String candidate=line.isEmpty()?word:line+" "+word;
    if(!line.isEmpty() && M5.Display.textWidth(candidate)>width) {
      M5.Display.drawString(line,centerX,y+count*lineHeight); ++count; line="";
      if(count>=maxLines) break;
    } else { line=candidate; remaining=split<0?"":remaining.substring(split+1); }
    if(split<0) remaining="";
  }
  if(count<maxLines && !line.isEmpty()) { if(!remaining.isEmpty()){while(!line.isEmpty()&&M5.Display.textWidth(line+"...")>width)line.remove(line.length()-1);line+="...";} M5.Display.drawString(line,centerX,y+count*lineHeight);++count; }
  return y+count*lineHeight;
}
void writeJsonString(File& file, const String& value) { JsonDocument document; document.set(value); serializeJson(document, file); }

String cardKey(const String& value) {
  // Card names are user/AI supplied. Fold case and spacing so "T. rex" and
  // " t. REX " are treated as the same saved fact rather than duplicates.
  String key(value); key.trim(); key.toLowerCase();
  while (key.indexOf("  ") >= 0) key.replace("  ", " ");
  return key;
}
}

String FlashcardsApp::deckPath() const { return String(kDeckFolder) + "/" + kCategoryFiles[category_]; }
const char* FlashcardsApp::categoryName() const { return kCategories[category_]; }
const char* FlashcardsApp::categoryLabel() const { return kCategoryLabels[category_]; }

bool FlashcardsApp::ensureStarterDeck(AppContext& context) {
  if (!context.storage.mounted()) { status_ = "Insert a microSD card"; return false; }
  SD.mkdir("/PaperOS");
  SD.mkdir(kDeckFolder);
  SD.mkdir(kImageFolder);
  const String path = deckPath();
  if (SD.exists(path)) return true;

  if (category_ != 0) {
    File empty = SD.open(path, FILE_WRITE);
    if (!empty) { status_ = "Could not create this category deck"; return false; }
    empty.print("{\"deck\":\""); empty.print(categoryName()); empty.print("\",\"cards\":[]}"); empty.close(); return true;
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) { status_ = "Could not create the Dinosaur deck"; return false; }
  file.print("{\"deck\":\"DINOSAURS\",\"cards\":[");
  for (uint16_t index = 0; index < kStarterCardCount; ++index) {
    if (index) file.print(',');
    file.print("{\"name\":\""); file.print(kDinosaurNames[index]);
    file.print("\",\"fact\":\""); file.print(groupFact(index)); file.print("\"}");
  }
  file.print("]}");
  file.close();
  return true;
}

bool FlashcardsApp::loadDeck(AppContext& context) {
  cardCount_ = current_ = 0;
  if (!ensureStarterDeck(context)) return false;
  File file = SD.open(deckPath(), FILE_READ);
  if (!file) { status_ = "Could not open the selected deck"; return false; }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, file);
  file.close();
  if (error) { status_ = "The selected deck is not valid JSON"; return false; }

  JsonArray cards = document["cards"].as<JsonArray>();
  for (JsonObject card : cards) {
    if (cardCount_ >= MaxCards) break;
    cards_[cardCount_].name = card["name"].as<const char*>();
    cards_[cardCount_].fact = card["fact"].as<const char*>();
    cards_[cardCount_].image = card["image"] | "";
    cards_[cardCount_].era = card["era"] | "";
    cards_[cardCount_].diet = card["diet"] | "";
    cards_[cardCount_].length = card["length"] | "";
    cards_[cardCount_].region = card["region"] | "";
    if (!cards_[cardCount_].name.isEmpty() && !cards_[cardCount_].fact.isEmpty()) ++cardCount_;
  }
  // Upgrade the original starter deck (name + fact only) with table values.
  // Imported/AI cards that already carry metadata are left unchanged.
  bool migrated=false;
  if(category_==0) for(uint16_t i=0;i<cardCount_;++i){Card& card=cards_[i];if(!card.era.isEmpty()||!card.diet.isEmpty()||!card.length.isEmpty()||!card.region.isEmpty())continue;
    card.era="Mesozoic Era";card.region="Fossils worldwide";
    if(i<60){card.diet="Theropod";card.length="Varies by species";}else if(i<100){card.diet="Plant-eater";card.length="Long-necked group";}else if(i<160){card.diet="Plant-eater";card.length="Varies by species";}else{card.diet="Dinosaur group";card.length="Varies by species";}migrated=true;}
  if(migrated)saveDeck(context);
  status_ = cardCount_ ? String(cardCount_) + " " + categoryLabel() + " cards on SD" : String("No cards in ") + categoryLabel();
  return cardCount_ > 0;
}

bool FlashcardsApp::saveDeck(AppContext& context) {
  if (!context.storage.mounted()) return false;
  const String path=deckPath(), temporary=path+".part";
  if(SD.exists(temporary))SD.remove(temporary); File file=SD.open(temporary,FILE_WRITE); if(!file)return false;
  file.print("{\"deck\":");writeJsonString(file,categoryName());file.print(",\"cards\":[");
  for(uint16_t i=0;i<cardCount_;++i){if(i)file.print(',');const Card& card=cards_[i];file.print("{\"name\":");writeJsonString(file,card.name);file.print(",\"fact\":");writeJsonString(file,card.fact);
    if(!card.image.isEmpty()){file.print(",\"image\":");writeJsonString(file,card.image);}if(!card.era.isEmpty()){file.print(",\"era\":");writeJsonString(file,card.era);}if(!card.diet.isEmpty()){file.print(",\"diet\":");writeJsonString(file,card.diet);}if(!card.length.isEmpty()){file.print(",\"length\":");writeJsonString(file,card.length);}if(!card.region.isEmpty()){file.print(",\"region\":");writeJsonString(file,card.region);}file.print('}');}
  file.print("]}");file.close();if(SD.exists(path))SD.remove(path);return SD.rename(temporary,path);
}

ui::ChromeOptions FlashcardsApp::chromeOptions() const {
  ui::ChromeOptions options;
  options.showBack = !categoryLanding_;
  // The category landing is the app's root, so Home exits Flashcards there.
  // Inside a deck, Back returns to categories and only Prev/Next navigate.
  options.showHome = categoryLanding_;
  options.showPrevious = !categoryLanding_ && current_ > 0;
  options.showNext = !categoryLanding_ && current_ + 1 < cardCount_;
  return options;
}

bool FlashcardsApp::onStart(AppContext& context) {
  categoryLanding_ = true;
  status_ = "Choose a category to begin";
  draw(context);
  return true;
}

void FlashcardsApp::showCard(AppContext& context, uint16_t index) {
  current_ = index;
  refreshCardContent(context);
}

void FlashcardsApp::showRandomCard(AppContext& context) {
  if (cardCount_ < 2) return;
  uint16_t next = current_;
  // Never present the same card twice in a row. The deck itself remains in
  // SD order, so PREV/NEXT are still useful for deliberate study.
  while (next == current_) next = static_cast<uint16_t>(esp_random() % cardCount_);
  showCard(context, next);
}

bool FlashcardsApp::drawCardImage(const String& relativePath, int x, int y) const {
  if (!cardImageAvailable(relativePath)) return false;
  const String path = String(kDeckFolder) + "/" + relativePath;
  String lower = path; lower.toLowerCase();
  // Card artwork should be exported at 180 x 130 pixels. Keeping decoding
  // bounded avoids a large bitmap allocation or a slow full-screen refresh.
  if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) M5.Display.drawJpgFile(SD, path.c_str(), x, y, 0, 0, 0, 0, 1.0f, 1.0f);
  else if (lower.endsWith(".png")) M5.Display.drawPngFile(SD, path.c_str(), x, y, 0, 0, 0, 0, 1.0f, 1.0f);
  else return false;
  return true;
}
bool FlashcardsApp::cardImageAvailable(const String& relativePath) const { if(relativePath.isEmpty()||relativePath.indexOf("..")>=0||relativePath.startsWith("/"))return false;String path=String(kDeckFolder)+"/"+relativePath;String lower=path;lower.toLowerCase();return SD.exists(path)&&(lower.endsWith(".jpg")||lower.endsWith(".jpeg")||lower.endsWith(".png")); }

void FlashcardsApp::drawActionBar(AppContext& context) {
  const int footer=ui::Chrome::footerTop(), y=footer-126;
  M5.Display.fillRect(0,y-8,M5.Display.width(),70,TFT_WHITE);
  actionButton(70,y,190,56,context.geminiFlashcards.inProgress()?"Working":"Generate");
  actionButton(280,y,190,56,"Random");
  (void)context;
}

void FlashcardsApp::drawStatus() {
  const int y=ui::Chrome::footerTop()-48;
  M5.Display.fillRect(20,y,500,30,TFT_WHITE); M5.Display.setFont(&fonts::FreeSans9pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString(trimText(status_,470),270,y+14);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
}

void FlashcardsApp::drawCardContent(AppContext& context) {
  const int header=ui::Chrome::headerHeight(), top=header+24, actionY=ui::Chrome::footerTop()-126, bottom=actionY-28;
  M5.Display.fillRect(0,header,M5.Display.width(),bottom-header,TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b); M5.Display.setTextDatum(ML_DATUM);
  M5.Display.drawString(String(categoryLabel()) + "  |  " + (cardCount_ ? String(current_+1)+" of "+String(cardCount_) : "Empty deck"),28,header+12);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
  if (!cardCount_) {
    M5.Display.setFont(&fonts::FreeSansBold18pt7b);M5.Display.setTextDatum(MC_DATUM);M5.Display.drawString("No " + String(categoryLabel()) + " cards",270,top+220);
    M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.drawString("Tap Generate to create and save one",270,top+270);M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
    return;
  }
  frame(28, top, M5.Display.width() - 56, bottom - top);
  const int centerX = M5.Display.width() / 2;
  const bool hasImage = drawCardImage(cards_[current_].image, centerX - 90, top + 26);
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  const int titleBottom=drawWrappedText(cards_[current_].name,centerX,top+(hasImage?228:56),M5.Display.width()-78,2,30);
  const Card& card = cards_[current_];
  // Keep both metadata cells strictly inside the 468px content column:
  // 229 + 10px gap + 229 = 468. The previous 235px cells protruded past the
  // parent card's right edge and made the frame look mismatched.
  const int tableY=max(titleBottom+22,top+(hasImage?292:112)), cellW=229, cellH=68, left=36, gap=10;
  tableCell(left,tableY,cellW,cellH,"PERIOD",card.era.isEmpty()?"Not available":card.era);
  tableCell(left+cellW+gap,tableY,cellW,cellH,"TYPE",card.diet.isEmpty()?"Not available":card.diet);
  tableCell(left,tableY+cellH+8,cellW,cellH,"SIZE",card.length.isEmpty()?"Not available":card.length);
  tableCell(left+cellW+gap,tableY+cellH+8,cellW,cellH,"PLACE",card.region.isEmpty()?"Not available":card.region);
  const int factY=tableY+cellH*2+28, factH=bottom-factY-20;
  ui::Theme::drawFrame(left, factY, 468, factH, 0);
  M5.Display.fillRect(left+3,factY+3,462,31,TFT_BLACK);M5.Display.setFont(&fonts::FreeSansBold12pt7b);M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);M5.Display.setTextDatum(ML_DATUM);M5.Display.drawString("FACT",left+16,factY+19);M5.Display.setTextColor(TFT_BLACK,TFT_WHITE);
  M5.Display.setFont(&fonts::FreeSans12pt7b);M5.Display.setTextDatum(MC_DATUM);
  drawWrappedText(card.fact,centerX,factY+58,430,max(3,(factH-70)/23),23);
  M5.Display.setTextDatum(TL_DATUM);M5.Display.setFont(nullptr);
  (void)context;
}

void FlashcardsApp::refreshCardContent(AppContext& context) {
  drawCardContent(context); drawStatus();
  M5.Display.display(0,ui::Chrome::headerHeight(),M5.Display.width(),ui::Chrome::footerTop()-ui::Chrome::headerHeight()-126);
  M5.Display.display(0,ui::Chrome::footerTop()-48,M5.Display.width(),30);
  // PREV/NEXT availability depends on the selected card. Refresh only the
  // shared navigation strip, not the header or large card area again.
  ui::Chrome::drawFooter(chromeOptions());
  M5.Display.display(0,ui::Chrome::footerTop(),M5.Display.width(),ui::Chrome::footerHeight());
}

void FlashcardsApp::draw(AppContext& context) {
  M5.Display.fillScreen(TFT_WHITE); ui::Chrome::drawHeader(context, "Flashcards");
  if (categoryLanding_) { drawCategoryLanding(context); ui::Chrome::drawFooter(chromeOptions()); return; }
  drawCardContent(context); drawStatus(); drawActionBar(context);
  // Always surface asynchronous generation state. Previously this was shown
  // only for an empty category, making a failed request look like no tap had
  // occurred when a deck already had cards.
  ui::Chrome::drawFooter(chromeOptions());
}

void FlashcardsApp::drawCategoryLanding(AppContext& context) {
  M5.Display.setFont(&fonts::FreeSans12pt7b); M5.Display.setTextDatum(MC_DATUM);
  M5.Display.drawString("Choose a category", 270, ui::Chrome::headerHeight() + 28);
  for (uint8_t i = 0; i < kCategoryCount; ++i) {
    const int row=i/2, col=i%2, x=42+col*238, y=ui::Chrome::headerHeight()+70+row*148;
    // Text-only buttons avoid low-detail icon noise and retain the shared
    // 3px frame on every edge.
    ui::Theme::drawButtonFrame(x,y,216,118);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b); M5.Display.drawString(kCategoryLabels[i], x+108, y+59);
  }
  M5.Display.setTextDatum(TL_DATUM); M5.Display.setFont(nullptr);
  (void)context;
}

void FlashcardsApp::onTick(AppContext& context, uint32_t) {
  GeneratedFlashcard generated;
  if (context.geminiFlashcards.consume(generated)) {
    if (generated.success) {
      const String key = cardKey(generated.name);
      int existing = -1;
      for (uint16_t i = 0; i < cardCount_; ++i) if (cardKey(cards_[i].name) == key) { existing = i; break; }
      if (key.isEmpty() || !generated.fact[0]) {
        status_ = "Generated card was missing a name or fact";
      } else if (existing >= 0) {
        current_ = static_cast<uint16_t>(existing);
        status_ = "Already saved: " + cards_[current_].name + " (" + String(cardCount_) + " cards)";
      } else if (cardCount_ >= MaxCards) {
        status_ = "Deck limit reached (" + String(MaxCards) + " cards)";
      } else {
        const uint16_t inserted = cardCount_++;
        Card& card = cards_[inserted];
        card.name=generated.name;card.fact=generated.fact;card.image=generated.image;card.era=generated.era;card.diet=generated.diet;card.length=generated.length;card.region=generated.region;
        if (saveDeck(context)) {
          current_=inserted;
          status_ = "Saved: " + card.name + " (" + String(cardCount_) + " cards)";
        } else {
          --cardCount_;
          status_ = "Could not save card to SD";
        }
      }
    } else status_=generated.message;
    if (!categoryLanding_) { drawActionBar(context); refreshCardContent(context); M5.Display.display(0,ui::Chrome::footerTop()-134,M5.Display.width(),116); }
    return;
  }
  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed() && !touch.wasReleased()) return;
  const auto action = touch.wasPressed() ? ui::Chrome::hitTestFooter(touch.x, touch.y, chromeOptions()) : ui::Chrome::swipeAction(touch.base_x,touch.base_y,touch.x,touch.y,chromeOptions());
  if (action == ui::FooterAction::Home) { if (navigator_) navigator_(AppId::Launcher); return; }
  if (action == ui::FooterAction::Back && !categoryLanding_) { categoryLanding_ = true; status_ = "Choose a category to begin"; draw(context); return; }
  if (action == ui::FooterAction::Previous && current_ > 0) { showCard(context, current_ - 1); return; }
  if (action == ui::FooterAction::Next && current_ + 1 < cardCount_) { showCard(context, current_ + 1); return; }
  if (categoryLanding_) {
    const int startY = ui::Chrome::headerHeight() + 70;
    if (touch.wasPressed() && touch.x >= 42 && touch.x <= 496 && touch.y >= startY && touch.y < startY + 3 * 148) {
      const uint8_t column = touch.x >= 280, row = (touch.y - startY) / 148, selected = row * 2 + column;
      const int cardX = 42 + column * 238, cardY = startY + row * 148;
      if (selected < kCategoryCount && touch.x <= cardX + 216 && touch.y <= cardY + 118) {
        category_ = selected; loadDeck(context);
        if (cardCount_) current_ = esp_random() % cardCount_;
        categoryLanding_ = false; draw(context);
      }
    }
    return;
  }
  const int actionY=ui::Chrome::footerTop()-126;
  if (touch.wasPressed() && touch.x >= 70 && touch.x <= 260 && touch.y >= actionY && touch.y <= actionY+56) {
    if(context.geminiFlashcards.request(categoryName(),context.settings))status_="Generating AI card…";
    else status_=context.settings.geminiApiKey().isEmpty()?"Add a Gemini key in AP Settings":"Connect Wi-Fi or wait for the current request";
    drawActionBar(context);drawStatus();M5.Display.display(0,ui::Chrome::footerTop()-134,M5.Display.width(),116);return;
  }
  if (touch.wasPressed() && touch.x >= 280 && touch.x <= 470 && touch.y >= actionY && touch.y <= actionY+56) { showRandomCard(context); return; }
}
