#include "services/Services.h"
#include <SD.h>
#include <SPI.h>
bool StorageService::begin(){SPI.begin(39,40,38,47);mounted_=SD.begin(47,SPI);if(mounted_){SD.mkdir("/PaperOS");SD.mkdir("/PaperOS/books");SD.mkdir("/PaperOS/photos");SD.mkdir("/PaperOS/notes");SD.mkdir("/PaperOS/todo");SD.mkdir("/PaperOS/flashcards");SD.mkdir("/PaperOS/fonts");SD.mkdir("/PaperOS/uploads");SD.mkdir("/PaperOS/backup");SD.mkdir("/PaperOS/cache");SD.mkdir("/PaperOS/chess");SD.mkdir("/PaperOS/pomodoro");SD.mkdir("/PaperOS/writing");SD.mkdir("/PaperOS/icons");SD.mkdir("/PaperOS/icons/weather");}return mounted_;}
bool StorageService::unmountForUsb(){if(mounted_)SD.end();mounted_=false;return true;}
bool StorageService::mountForFirmware(){return begin();}

bool StorageService::prepareCacheWrite(size_t bytesNeeded) {
  if (!mounted_ || bytesNeeded == 0) return false;
  constexpr uint64_t kReserveBytes = 512UL * 1024UL;
  const uint64_t needed = static_cast<uint64_t>(bytesNeeded) + kReserveBytes;
  if (SD.totalBytes() >= SD.usedBytes() && SD.totalBytes() - SD.usedBytes() >= needed) return true;
  // Cache entries are regenerable. Delete only the cache namespace rather
  // than touching user books/photos or accepting a partial cache write.
  File directory = SD.open("/PaperOS/cache");
  if (directory && directory.isDirectory()) {
    for (File entry = directory.openNextFile(); entry; entry = directory.openNextFile()) {
      const String name = entry.name(); const bool isDirectory = entry.isDirectory(); entry.close();
      if (!isDirectory) SD.remove(name);
    }
  }
  if (directory) directory.close();
  return SD.totalBytes() >= SD.usedBytes() && SD.totalBytes() - SD.usedBytes() >= needed;
}
