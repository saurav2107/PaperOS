#!/usr/bin/env python3
"""Create SD-ready PaperOS flashcard decks with Gemini in small batches.

Example:
  GEMINI_API_KEY=... python3 tools/generate_flashcard_decks.py --output /Volumes/SDCARD/PaperOS/flashcards
"""
import argparse, json, os, time, urllib.request

CATEGORIES = ("DINOSAURS", "SPACE", "ANIMALS", "SCIENCE", "GEOGRAPHY", "HISTORY")

def call_gemini(key, model, category, start, count):
    prompt = f'''Return only a JSON array of exactly {count} factual, unique educational flashcards for {category}, numbered conceptually from {start}.
Each object must have name, fact (45-65 words), era, diet, length, region, image_prompt, image.
Set image to an empty string. Use the field meanings appropriate to the category: for Space, diet means object class; for Science, scientific field; for Geography, feature type; for History, person/event type. Never use Markdown.'''
    payload = {"contents":[{"parts":[{"text":prompt}]}], "generationConfig":{"responseMimeType":"application/json","maxOutputTokens":4096,"thinkingConfig":{"thinkingLevel":"minimal"}}}
    request = urllib.request.Request(f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent", data=json.dumps(payload).encode(), headers={"x-goog-api-key":key,"Content-Type":"application/json"})
    with urllib.request.urlopen(request, timeout=90) as response:
        body=json.load(response)
    text=body["candidates"][0]["content"]["parts"][-1]["text"]
    return json.loads(text[text.index("["):text.rindex("]")+1])

def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--api-key", default=os.environ.get("GEMINI_API_KEY")); parser.add_argument("--model",default="gemini-3.6-flash"); parser.add_argument("--output",required=True); parser.add_argument("--count",type=int,default=200); args=parser.parse_args()
    if not args.api_key: parser.error("set GEMINI_API_KEY or pass --api-key")
    os.makedirs(args.output,exist_ok=True)
    for category in CATEGORIES:
        cards=[]
        for start in range(1,args.count+1,10):
            batch=call_gemini(args.api_key,args.model,category,start,min(10,args.count-start+1))
            cards.extend(batch); print(f"{category}: {len(cards)}/{args.count}")
            time.sleep(1.2)
        path=os.path.join(args.output,category.lower()+".json")
        with open(path,"w",encoding="utf-8") as file: json.dump({"deck":category,"cards":cards[:args.count]},file,ensure_ascii=False,indent=2)
        print("wrote",path)

if __name__ == "__main__": main()
