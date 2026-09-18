#include "platform/ai/GeminiFlashcardService.h"
#include "services/Services.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <SD.h>
#include <mbedtls/base64.h>

namespace {
struct Request { GeminiFlashcardService* service; String category, key, textModel, imageModel; };
void message(GeneratedFlashcard& result, const char* text) { strlcpy(result.message, text, sizeof(result.message)); }
void copy(char* target, size_t size, const String& value) { strlcpy(target, value.c_str(), size); }
String endpoint(const String& model) { return "https://generativelanguage.googleapis.com/v1beta/models/" + model + ":generateContent"; }
String categoryFieldGuide(const String& category) {
  if (category=="DINOSAURS") return "Use era=geological period, diet=feeding type, length=body length, region=fossil region.";
  if (category=="SPACE") return "Use era=formation/discovery period, diet=object class, length=diameter or scale, region=solar-system/galactic location.";
  if (category=="ANIMALS") return "Use era=present-day or evolutionary period, diet=feeding type, length=typical adult size, region=natural habitat.";
  if (category=="SCIENCE") return "Use era=discovery period, diet=scientific field, length=scale/unit where meaningful, region=origin or real-world context.";
  if (category=="GEOGRAPHY") return "Use era=formation/history period, diet=geographic feature type, length=size/elevation/distance, region=country or location.";
  if (category=="HISTORY") return "Use era=historical period/date, diet=person/event type, length=duration or scale, region=place/civilization.";
  return "Use the closest accurate meaning for every required field.";
}
bool postJson(const String& model, const String& key, const String& body, String& response, int& status) {
  // This function is called from the dedicated worker, never loopTask. A
  // couple of bounded retries handle transient AP/router TLS failures without
  // freezing touch input or retrying quota/key errors.
  for (uint8_t attempt=0; attempt<3; ++attempt) {
    response=""; WiFiClientSecure client; client.setInsecure(); client.setTimeout(20000); client.setHandshakeTimeout(20);
    HTTPClient http; http.setTimeout(20000); http.useHTTP10(true); http.setReuse(false);
    if (!http.begin(client, endpoint(model))) { status=-1; Serial.printf("[GEMINI] begin failed for %s\n", model.c_str()); }
    else {
      http.addHeader("x-goog-api-key", key); http.addHeader("Content-Type", "application/json"); http.addHeader("Accept-Encoding", "identity");
      status=http.POST(body); const int length=http.getSize();
      if (length > 180 * 1024) { Serial.printf("[GEMINI] %s response too large: %d bytes\n",model.c_str(),length); http.end(); status=413; return false; }
      if (status > 0) response=http.getString();
      Serial.printf("[GEMINI] %s attempt %u HTTP %d, response %u bytes\n", model.c_str(), static_cast<unsigned>(attempt+1), status, static_cast<unsigned>(response.length()));
      http.end();
    }
    if (status==HTTP_CODE_OK) return true;
    // Never re-send invalid, forbidden, quota-limited, or oversized requests.
    if (status==400 || status==401 || status==403 || status==404 || status==413 || status==429) return false;
    if (attempt<2) { const uint32_t waitMs=1000UL << attempt; Serial.printf("[GEMINI] transient network failure; retrying in %lu ms\n",static_cast<unsigned long>(waitMs)); vTaskDelay(pdMS_TO_TICKS(waitMs)); }
  }
  return false;
}
String textFromResponse(const String& response) {
  JsonDocument document; if (deserializeJson(document, response)) return "";
  const char* finishReason=document["candidates"][0]["finishReason"]|"UNKNOWN";
  Serial.printf("[GEMINI] finish reason: %s\n",finishReason);
  // Gemini 3.x thinking models may emit one or more internal thought parts
  // before the actual answer. The old implementation always selected part 0,
  // which made a successful HTTP response look like malformed card JSON.
  String answer, fallback;
  JsonArray parts = document["candidates"][0]["content"]["parts"].as<JsonArray>();
  for (JsonVariant part : parts) {
    const char* value = part["text"].as<const char*>();
    if (!value || !value[0]) continue;
    const String text(value);
    // A structured card always contains a JSON object. Prefer it even if a
    // provider marks its enclosing part as thought differently than expected.
    if (text.indexOf('{') >= 0 && text.lastIndexOf('}') > text.indexOf('{')) answer = text;
    else if (!(part["thought"].is<bool>() && part["thought"].as<bool>())) fallback = text;
  }
  if (answer.isEmpty()) answer = fallback;
  answer.trim();
  // Keep compatibility with a user-selected model that disregards JSON mode
  // and wraps the otherwise valid object in a Markdown code fence.
  const int firstObject = answer.indexOf('{'), lastObject = answer.lastIndexOf('}');
  if (firstObject >= 0 && lastObject > firstObject) answer = answer.substring(firstObject, lastObject + 1);
  if (answer.isEmpty()) Serial.printf("[GEMINI] no final text part; response preview: %.500s\n",response.c_str());
  return answer;
}
String apiError(const String& response) {
  JsonDocument document; if(deserializeJson(document,response)) return "";
  const String value=document["error"]["message"]|"";
  return value.substring(0,70);
}
}

bool GeminiFlashcardService::request(const String& category, const DeviceSettingsService& settings) {
  if (inProgress_ || settings.geminiApiKey().isEmpty() || WiFi.status()!=WL_CONNECTED) return false;
  Request* request=new Request{this,category,settings.geminiApiKey(),settings.geminiTextModel(),settings.geminiImageModel()};
  if (!request) return false;
  Serial.printf("[GEMINI] queued %s card using text=%s image=%s\n",category.c_str(),settings.geminiTextModel().c_str(),settings.geminiImageModel().c_str());
  inProgress_=true;
  if (xTaskCreatePinnedToCore(worker,"GeminiCard",12288,request,1,&task_,0)!=pdPASS) { delete request; inProgress_=false; return false; }
  return true;
}
void GeminiFlashcardService::worker(void* argument) { Request* request=static_cast<Request*>(argument); request->service->run(request->category,request->key,request->textModel,request->imageModel); delete request; vTaskDelete(nullptr); }

void GeminiFlashcardService::run(const String& category, const String& key, const String& textModel, const String& imageModel) {
  GeneratedFlashcard output{};
  JsonDocument prompt;
  const String instruction =
    "Create exactly one factual educational flashcard in the category '" + category +
    "'. Return ONLY one valid JSON object: no Markdown, code fences, commentary, "
    "citations, or extra keys. Required schema: "
    "{\"name\":\"string (max 70 chars)\",\"fact\":\"string (45-65 words)\","
    "\"era\":\"string (max 40 chars)\",\"diet\":\"string (max 40 chars)\","
    "\"length\":\"string (max 40 chars)\",\"region\":\"string (max 70 chars)\","
    "\"image_prompt\":\"string (max 180 chars)\"}. "
    "Use accurate, neutral, child-friendly information. Do not invent disputed facts. " + categoryFieldGuide(category);
  prompt["contents"][0]["parts"][0]["text"]=instruction;
  prompt["generationConfig"]["responseMimeType"]="application/json";
  // Flashcards are structured extraction, not a reasoning task. Gemini 3.6
  // defaults to medium thinking; minimal leaves output room for final JSON and
  // avoids MAX_TOKENS from an otherwise successful request.
  prompt["generationConfig"]["thinkingConfig"]["thinkingLevel"]="minimal";
  prompt["generationConfig"]["maxOutputTokens"]=1024;
  String body; serializeJson(prompt,body); String response; int status=0;
  if (!postJson(textModel,key,body,response,status)) {
    if(status==429) message(output,"AI LIMIT REACHED — TRY LATER OR USE PAID MODEL");
    else if(status==401||status==403) message(output,"GEMINI API KEY WAS REJECTED");
    else if(status==413) message(output,"AI RESPONSE TOO LARGE");
    else if(status<=0) message(output,"AI NETWORK FAILED — CHECK WI-FI AND TRY AGAIN");
    else { const String detail=apiError(response); snprintf(output.message,sizeof(output.message),"AI REQUEST %d%s%s",status,detail.isEmpty()?"":" ",detail.c_str()); }
  } else {
    const String raw=textFromResponse(response); JsonDocument card;
    if(raw.isEmpty()||deserializeJson(card,raw)) { Serial.printf("[GEMINI] final text was not card JSON (%u bytes): %.160s\n",static_cast<unsigned>(raw.length()),raw.c_str()); message(output,"AI RETURNED INVALID CARD DATA"); }
    else {
      copy(output.name,sizeof(output.name),card["name"]|""); copy(output.fact,sizeof(output.fact),card["fact"]|""); copy(output.era,sizeof(output.era),card["era"]|""); copy(output.diet,sizeof(output.diet),card["diet"]|""); copy(output.length,sizeof(output.length),card["length"]|""); copy(output.region,sizeof(output.region),card["region"]|"");
      if(!output.name[0]||!output.fact[0]) message(output,"AI CARD IS MISSING REQUIRED FACTS");
      else {
        output.success=true; message(output,"CARD SAVED TO SD");
        // Image generation is optional. A text card is saved even when the
        // selected image model is unavailable on the user's free tier.
        JsonDocument imagePrompt;
        imagePrompt["contents"][0]["parts"][0]["text"] =
          "Create one final PNG illustration for a 540x960 monochrome e-ink device. Subject: " + String(card["image_prompt"]|output.name) +
          ". Strict requirements: 180x130 pixels, black ink on pure white background, simple high-contrast line art, no grayscale, no shadows, no gradients, no border, no letters, no labels, no watermark, centered subject, and keep the PNG below 100 KB.";
        imagePrompt["generationConfig"]["responseModalities"][0]="IMAGE";
        String imageBody; serializeJson(imagePrompt,imageBody); String imageResponse; int imageStatus=0;
        if(postJson(imageModel,key,imageBody,imageResponse,imageStatus)) {
          JsonDocument imageJson; if(!deserializeJson(imageJson,imageResponse)) {
            const char* data=nullptr; const char* mime="image/png";
            for(JsonObject part : imageJson["candidates"][0]["content"]["parts"].as<JsonArray>()) { const char* candidate=part["inlineData"]["data"]|nullptr; if(candidate){data=candidate;mime=part["inlineData"]["mimeType"]|"image/png";break;} }
            if(data) { size_t decoded=0; mbedtls_base64_decode(nullptr,0,&decoded,reinterpret_cast<const unsigned char*>(data),strlen(data));
              if(decoded>0&&decoded<=100UL*1024UL){uint8_t* bytes=static_cast<uint8_t*>(ps_malloc(decoded));size_t written=0;if(bytes&&mbedtls_base64_decode(bytes,decoded,&written,reinterpret_cast<const unsigned char*>(data),strlen(data))==0){SD.mkdir("/PaperOS");SD.mkdir("/PaperOS/flashcards");SD.mkdir("/PaperOS/flashcards/images");const String extension=String(mime).indexOf("jpeg")>=0?".jpg":".png";const String path=String("/PaperOS/flashcards/images/ai_")+String(millis())+extension;const String temporary=path+".part";File f=SD.open(temporary,FILE_WRITE);if(f&&f.write(bytes,written)==written){f.close();if(SD.exists(path))SD.remove(path);if(SD.rename(temporary,path)){copy(output.image,sizeof(output.image),String("images/")+path.substring(path.lastIndexOf('/')+1));output.imageSaved=true;}}else if(f)f.close();if(SD.exists(temporary))SD.remove(temporary);}if(bytes)free(bytes);}
            }
          }
        } else Serial.printf("[GEMINI] image skipped/failed with HTTP %d; text card remains valid\n",imageStatus);
      }
    }
  }
  portENTER_CRITICAL(&lock_); result_=output; ready_=true; inProgress_=false; task_=nullptr; portEXIT_CRITICAL(&lock_);
}
bool GeminiFlashcardService::consume(GeneratedFlashcard& result){bool available=false;portENTER_CRITICAL(&lock_);if(ready_){result=result_;ready_=false;available=true;}portEXIT_CRITICAL(&lock_);return available;}
