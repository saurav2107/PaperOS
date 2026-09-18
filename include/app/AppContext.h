#pragma once

class DisplayService;
class DeviceSettingsService;
class InputService;
class UiSoundService;
class StorageService;
class NetworkService;
class TimeService;
class PowerService;
class TaskRegistry;
class WeatherSettingsService;
class TemperatureService;
class ClockFaceService;
class PomodoroStyleService;
class WeatherFaceService;
class WeatherFetchService;
class GeminiFlashcardService;

struct AppContext {
  DisplayService& display;
  DeviceSettingsService& settings;
  InputService& input;
  UiSoundService& uiSound;
  StorageService& storage;
  NetworkService& network;
  TimeService& time;
  PowerService& power;
  WeatherSettingsService& weatherSettings;
  TemperatureService& temperature;
  ClockFaceService& clockFace;
  PomodoroStyleService& pomodoroStyle;
  WeatherFaceService& weatherFace;
  WeatherFetchService& weatherFetch;
  GeminiFlashcardService& geminiFlashcards;
  TaskRegistry& tasks;
};
