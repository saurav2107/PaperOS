#include "services/Services.h"
#include <M5Unified.h>
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include "config/Location.h"
void TimeService::begin(){
  const String saved = settings_ ? settings_->timezone() : String(kHomeLocation.timezone);
  use24Hour_ = settings_ ? settings_->use24Hour() : true;
  saved.toCharArray(timezone_, sizeof(timezone_)); applyTimezone();
  // M5.begin() has already initialized M5.Rtc and seeded system time from it
  // when the PaperS3 RTC is available. Keep the capability for later writes.
  hardwareRtc_ = M5.Rtc.isEnabled();
  strlcpy(buffer_, "Time unavailable", sizeof(buffer_));
  // A blank header date is quieter and less distracting than an error-like
  // placeholder while the RTC/NTP clock has not supplied a date yet.
  dateBuffer_[0] = '\0';
  refreshCache();
}
void TimeService::refreshCache(){
  tm local{};
  // A zero-timeout sample never stalls the UI while Wi-Fi/NTP is unavailable.
  if(!getLocalTime(&local,0)) return;
  strftime(buffer_,sizeof(buffer_),use24Hour_ ? "%a %d %b  %H:%M" : "%a %d %b  %I:%M %p",&local);
  strftime(dateBuffer_,sizeof(dateBuffer_),"%d. %B",&local);
}
void TimeService::tick(){
  const uint32_t now=millis();
  if(now>=nextClockRefreshMs_){refreshCache();nextClockRefreshMs_=now+15000UL;}
  if (syncInProgress_) {
    tm value{};
    if (getLocalTime(&value, 0)) { writeHardwareRtc(value); syncInProgress_=false; lastSyncSucceeded_=true; }
    else if (static_cast<int32_t>(now - syncDeadlineMs_) >= 0) { syncInProgress_=false; lastSyncSucceeded_=false; }
  }
}
const char* TimeService::formattedLocalTime(){return buffer_;}
const char* TimeService::formattedDate(){return dateBuffer_;}
bool TimeService::localTime(tm& value) const { return getLocalTime(&value, 10); }
void TimeService::applyTimezone(){ configTzTime(timezone_, "pool.ntp.org", "time.google.com"); }
void TimeService::setTimezone(const char* timezone){
  if (!timezone || !timezone[0]) return;
  strlcpy(timezone_, timezone, sizeof(timezone_));
  if (settings_) settings_->setTimezone(timezone_);
  applyTimezone();
}
bool TimeService::syncFromInternet(){
  return requestInternetSync();
}
bool TimeService::requestInternetSync(){
  if (WiFi.status() != WL_CONNECTED || syncInProgress_) return false;
  applyTimezone(); lastSyncSucceeded_=false; syncInProgress_=true; syncDeadlineMs_=millis()+15000UL; return true;
}
void TimeService::setUse24Hour(bool value) {
  use24Hour_ = value;
  if (settings_) settings_->setUse24Hour(use24Hour_);
  refreshCache();
}
void TimeService::formatTime(const tm& value, char* target, size_t targetSize, bool seconds) const {
  if (!target || targetSize == 0) return;
  strftime(target, targetSize, use24Hour_ ? (seconds ? "%H:%M:%S" : "%H:%M") : (seconds ? "%I:%M:%S %p" : "%I:%M %p"), &value);
}
bool TimeService::setManualLocalTime(const tm& local) {
  tm copy = local;
  copy.tm_isdst = -1;
  const time_t epoch = mktime(&copy);
  if (epoch < 0) return false;
  timeval value{epoch, 0};
  if (settimeofday(&value, nullptr) != 0) return false;
  tm verified{}; localtime_r(&epoch, &verified); writeHardwareRtc(verified);
  refreshCache();
  return true;
}
void TimeService::writeHardwareRtc(const tm& local) {
  if (hardwareRtc_) M5.Rtc.setDateTime(&local);
}
