#include "services/Services.h"
#include <Preferences.h>

void WeatherFaceService::begin(){Preferences p;p.begin("paperos",true);const uint8_t value=p.getUChar("weatherFace",0);p.end();face_=value<=static_cast<uint8_t>(WeatherFace::WeatherClock)?static_cast<WeatherFace>(value):WeatherFace::Dashboard;}
void WeatherFaceService::save(){Preferences p;p.begin("paperos",false);p.putUChar("weatherFace",static_cast<uint8_t>(face_));p.end();}
void WeatherFaceService::next(){face_=static_cast<WeatherFace>((static_cast<uint8_t>(face_)+1)%8);save();}
const char* WeatherFaceService::name() const {switch(face_){case WeatherFace::Minimal:return "Minimal";case WeatherFace::Hourly:return "Hourly";case WeatherFace::Daily:return "Daily";case WeatherFace::TodayCard:return "Today Card";case WeatherFace::Timeline:return "Timeline";case WeatherFace::ThreeDay:return "3-Day";case WeatherFace::WeatherClock:return "Weather Clock";default:return "Dashboard";}}
