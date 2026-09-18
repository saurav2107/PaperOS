#include "services/Services.h"
#include <WebServer.h>
#include <WiFi.h>
#include <SD.h>
#include "config/Location.h"

namespace {
WebServer setupServer(80);
bool serverStarted = false;
DeviceSettingsService* deviceSettings = nullptr;
String escapeHtml(const String& value) {
  String result; result.reserve(value.length() + 12);
  for (size_t i=0; i<value.length(); ++i) {
    const char c=value[i];
    if(c=='&') result += "&amp;"; else if(c=='<') result += "&lt;";
    else if(c=='>') result += "&gt;"; else if(c=='\'') result += "&#39;";
    else if(c=='\"') result += "&quot;"; else result += c;
  }
  return result;
}
String networkOptions(int& count) {
  // Never block the touch/UI loop with a foreground Wi-Fi scan. The scan is
  // launched asynchronously and the browser can refresh once results arrive.
  count = WiFi.scanComplete();
  if (count == WIFI_SCAN_FAILED || count == WIFI_SCAN_RUNNING) {
    if (count == WIFI_SCAN_FAILED) WiFi.scanNetworks(true, true);
    count = 0;
    return "<option value=''>Scanning nearby networks… refresh shortly</option>";
  }
  String html = "<option value=''>Select a scanned network</option>";
  if (count <= 0) return html;
  for(int i=0;i<count;++i) {
    const String ssid=WiFi.SSID(i);
    const String name=ssid.isEmpty()?"Hidden network":ssid;
    html += "<option value='" + escapeHtml(ssid) + "'>" + escapeHtml(name) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }
  return html;
}
String numberOptions(uint8_t selected) { const uint8_t v[]={0,1,5,10,15,30};String out;for(uint8_t n:v)out+="<option"+String(n==selected?" selected":"")+">"+String(n)+"</option>";return out; }
String fileList(const char* path) { if(!SD.exists(path)) return "No files yet.";File d=SD.open(path);String out;for(File f=d.openNextFile();f&&out.length()<1200;f=d.openNextFile()){out+="<li>"+escapeHtml(String(f.name()))+"</li>";}return out.isEmpty()?"No files yet.":"<ul>"+out+"</ul>"; }
File portalUpload;
String uploadTempPath, uploadFinalPath, uploadError;
size_t uploadLimit = 0, uploadWritten = 0;
bool uploadFailed = false;
void handleUpload(const char* folder, size_t limit) {
  HTTPUpload& u=setupServer.upload();
  if (u.status==UPLOAD_FILE_START) {
    uploadFailed=false; uploadWritten=0; uploadLimit=limit; uploadError="";
    String n=u.filename; n.replace("/","_"); n.replace("\\","_");
    if (n.isEmpty() || n.indexOf("..")>=0 || !SD.cardType()) { uploadFailed=true; uploadError="SD card is unavailable"; return; }
    if (!SD.exists("/PaperOS")) SD.mkdir("/PaperOS"); if(!SD.exists(folder))SD.mkdir(folder);
    uploadFinalPath=String(folder)+"/"+n; uploadTempPath=uploadFinalPath+".part";
    if (SD.exists(uploadTempPath)) SD.remove(uploadTempPath);
    if (SD.totalBytes()-SD.usedBytes() < limit/8 + 65536UL) { uploadFailed=true; uploadError="Not enough free SD space"; return; }
    portalUpload=SD.open(uploadTempPath,FILE_WRITE); if(!portalUpload){uploadFailed=true;uploadError="Could not create upload file";}
  } else if(u.status==UPLOAD_FILE_WRITE && portalUpload && !uploadFailed) {
    if(uploadWritten+u.currentSize>uploadLimit || portalUpload.write(u.buf,u.currentSize)!=u.currentSize){uploadFailed=true;uploadError="Upload exceeds limit or SD write failed";} else uploadWritten+=u.currentSize;
  } else if(u.status==UPLOAD_FILE_END) {
    if(portalUpload) portalUpload.close();
    if(uploadFailed || !uploadWritten) { if(!uploadTempPath.isEmpty()&&SD.exists(uploadTempPath))SD.remove(uploadTempPath); return; }
    if(SD.exists(uploadFinalPath)) SD.remove(uploadFinalPath);
    if(!SD.rename(uploadTempPath,uploadFinalPath)){uploadFailed=true;uploadError="Could not finalize upload"; if(SD.exists(uploadTempPath))SD.remove(uploadTempPath);}
  } else if(u.status==UPLOAD_FILE_ABORTED) { if(portalUpload)portalUpload.close(); if(!uploadTempPath.isEmpty()&&SD.exists(uploadTempPath))SD.remove(uploadTempPath); uploadFailed=true;uploadError="Upload cancelled"; }
}
void uploadReply(){ const String message=uploadFailed?"Upload failed: "+uploadError:"Upload saved safely. Reloading settings…"; setupServer.sendHeader("Location","/",true);setupServer.send(303,"text/plain",message); }
void savePortal(){
  if(!deviceSettings){setupServer.send(500,"text/plain","Settings service unavailable");return;}
  const String manual=setupServer.arg("manual_ssid"), selected=setupServer.arg("ssid"), ssid=manual.isEmpty()?selected:manual;
  if(!ssid.isEmpty())deviceSettings->setWifi(ssid,setupServer.arg("password"));
  DeviceLocation location=deviceSettings->location(); location.address=setupServer.arg("address"); location.city=setupServer.arg("city"); location.country=setupServer.arg("country");
  const String latitude=setupServer.arg("latitude"), longitude=setupServer.arg("longitude"); if(!latitude.isEmpty())location.latitude=latitude.toDouble(); if(!longitude.isEmpty())location.longitude=longitude.toDouble(); deviceSettings->setLocation(location);
  deviceSettings->setTimezone(setupServer.arg("timezone"));deviceSettings->setUse24Hour(setupServer.arg("clock24h")=="on");deviceSettings->setInactivityMinutes(setupServer.arg("idle").toInt());deviceSettings->setWeatherRetryMinutes(setupServer.arg("weather_retry").toInt());deviceSettings->setButtonSound(setupServer.arg("sound")=="on");
  deviceSettings->setGemini(setupServer.arg("gemini_key"), setupServer.arg("gemini_text_model"), setupServer.arg("gemini_image_model"));
  if(!ssid.isEmpty())WiFi.begin(ssid.c_str(),setupServer.arg("password").c_str());setupServer.sendHeader("Location","/",true);setupServer.send(303,"text/plain","");}
void showPortal(){
  int count=0; const String options=networkOptions(count);
  String html=R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><style>body{font:16px system-ui;margin:auto;max-width:680px;padding:16px;background:#f5f5f2;color:#151515}h1{margin:.2rem 0}.card{background:#fff;border:2px solid #181818;border-radius:14px;padding:16px;margin:14px 0}label{display:block;font-weight:700;margin:.7rem 0 .25rem}input,select{box-sizing:border-box;width:100%;padding:13px;border:2px solid #222;border-radius:9px;font-size:16px}button{width:100%;padding:14px;margin-top:16px;border:0;border-radius:9px;background:#111;color:#fff;font-size:16px;font-weight:700}.note{color:#444;line-height:1.45}</style><h1>Paper OS Setup</h1><p class="note">AP mode is active. Make changes below, then save.</p><form method="post" action="/save"><section class="card"><h2>Wi-Fi</h2><label>Scanned network</label><select name="ssid">)HTML";
  html+=options; html+=R"HTML(</select><label>Or type Wi-Fi / hotspot name</label><input name="manual_ssid"><label>Password</label><input name="password" type="password"></section><section class="card"><h2>Home & Location</h2><label>Address</label><input name="address" value=")HTML";
  const DeviceLocation location=deviceSettings?deviceSettings->location():DeviceLocation{};
  html+=escapeHtml(location.address); html+=R"HTML("><label>City</label><input name="city" value=")HTML"; html+=escapeHtml(location.city); html+=R"HTML("><label>Country</label><input name="country" value=")HTML"; html+=escapeHtml(location.country); html+=R"HTML("><label>Latitude</label><input name="latitude" inputmode="decimal" value=")HTML";html+=String(location.latitude,5);html+=R"HTML("><label>Longitude</label><input name="longitude" inputmode="decimal" value=")HTML";html+=String(location.longitude,5);html+=R"HTML("></section><section class="card"><h2>Clock, Weather & Power</h2><label>Timezone (POSIX)</label><input name="timezone" value=")HTML"; html+=escapeHtml(deviceSettings?deviceSettings->timezone():String(kHomeLocation.timezone));
  html += String(R"HTML("><label><input type="checkbox" name="clock24h")HTML") + (deviceSettings&&deviceSettings->use24Hour()?" checked":"") + R"HTML(> Use 24-hour time</label><label>Sleep after minutes</label><select name="idle">)HTML" + numberOptions(deviceSettings?deviceSettings->inactivityMinutes():5) + R"HTML(</select><label>Weather offline retry (minutes)</label><select name="weather_retry">)HTML" + numberOptions(deviceSettings?deviceSettings->weatherRetryMinutes():5) + String(R"HTML(</select><label><input type="checkbox" name="sound")HTML") + (deviceSettings&&deviceSettings->buttonSound()?" checked":"") + R"HTML(> Button sounds</label></section><section class="card"><h2>Flashcard AI (Gemini)</h2><p class="note">The key stays on this device. Leave it blank to retain the existing key. Image models may require a paid Gemini tier.</p><label>Gemini API key</label><input name="gemini_key" type="password" placeholder=")HTML" + (deviceSettings&&!deviceSettings->geminiApiKey().isEmpty()?"Saved — enter a replacement to change":"Paste Gemini API key") + R"HTML("><label>Text model</label><input name="gemini_text_model" value=")HTML" + escapeHtml(deviceSettings?deviceSettings->geminiTextModel():String("gemini-2.5-flash")) + R"HTML("><label>Image model</label><input name="gemini_image_model" value=")HTML" + escapeHtml(deviceSettings?deviceSettings->geminiImageModel():String("gemini-3.1-flash-image")) + R"HTML("></section><button>Save Device Settings</button></form><section class="card"><h2>SD Card Updates</h2><p class="note">Each upload is size-limited, written to a temporary file, then atomically renamed. Fonts save to <code>/PaperOS/fonts</code>; general files save to <code>/PaperOS/uploads</code>.</p><form method="post" action="/upload/font" enctype="multipart/form-data"><input type="file" name="file" accept=".ttf,.otf,.vlw"><button>Upload Font (max 2 MB)</button></form><form method="post" action="/upload/file" enctype="multipart/form-data"><input type="file" name="file"><button>Upload File to SD (max 8 MB)</button></form></section>)HTML";
  setupServer.send(200,"text/html",html);
}
}

void NetworkService::begin(){
  deviceSettings=settings_; const String ssid=settings_?settings_->ssid():String();const String password=settings_?settings_->password():String();
  WiFi.mode(WIFI_STA);
  // Let the ESP32 Wi-Fi stack recover in the background. Credentials are
  // stored explicitly in Preferences, so avoid the SDK writing flash on every
  // connect attempt.
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  if(!ssid.isEmpty()){hasStationCredentials_=true;WiFi.begin(ssid.c_str(),password.c_str());return;}
#if __has_include("config/Secrets.h")
#include "config/Secrets.h"
  hasStationCredentials_=true;
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
#endif
}
void NetworkService::tick(){
  if(provisioning_){const uint32_t now=millis();if(now>=nextPortalServiceMs_){setupServer.handleClient();nextPortalServiceMs_=now+15;}return;}
  // Auto-reconnect normally handles this. This is a low-frequency fallback
  // for AP/router changes without a blocking Wi-Fi scan in the UI loop.
  const uint32_t now=millis();
  if(hasStationCredentials_ && WiFi.status()!=WL_CONNECTED && now>=nextReconnectMs_){
    WiFi.reconnect();
    nextReconnectMs_=now+30000UL;
  }
  if(WiFi.status()==WL_CONNECTED) nextReconnectMs_=now+30000UL;
}
bool NetworkService::connected()const{return WiFi.status()==WL_CONNECTED;}
int32_t NetworkService::signalStrength() const { return connected() ? WiFi.RSSI() : -100; }
String NetworkService::configuredSsid() const { return settings_ ? settings_->ssid() : String(); }
String NetworkService::configuredAddress() const { return settings_ ? settings_->location().address : String(kHomeLocation.label); }
void NetworkService::startScan() {
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
  WiFi.scanDelete();
  WiFi.scanNetworks(true, true);
}
bool NetworkService::scanInProgress() const { return WiFi.scanComplete() == WIFI_SCAN_RUNNING; }
int NetworkService::scanResultCount() const {
  const int count = WiFi.scanComplete();
  return count > 0 ? count : 0;
}
String NetworkService::scanSsid(uint8_t index) const { return WiFi.SSID(index); }
int32_t NetworkService::scanRssi(uint8_t index) const { return WiFi.RSSI(index); }
void NetworkService::connect(const String& ssid, const String& password) {
  if (ssid.isEmpty()) return;
  if (provisioning_) stopProvisioning();
  if (settings_) settings_->setWifi(ssid, password);
  hasStationCredentials_ = true;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  WiFi.begin(ssid.c_str(), password.c_str());
  nextReconnectMs_ = millis() + 30000UL;
}
void NetworkService::beginPortal(){if(serverStarted)return;setupServer.on("/",HTTP_GET,showPortal);setupServer.on("/save",HTTP_POST,savePortal);setupServer.on("/upload/font",HTTP_POST,uploadReply,[]{handleUpload("/PaperOS/fonts",2UL*1024UL*1024UL);});setupServer.on("/upload/file",HTTP_POST,uploadReply,[]{handleUpload("/PaperOS/uploads",8UL*1024UL*1024UL);});setupServer.begin();serverStarted=true;}
void NetworkService::startProvisioning(){WiFi.mode(WIFI_AP_STA);WiFi.softAP(apSsid(),apPassword());WiFi.scanDelete();WiFi.scanNetworks(true,true);beginPortal();nextPortalServiceMs_=0;provisioning_=true;}
void NetworkService::stopProvisioning(){provisioning_=false;WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);}
