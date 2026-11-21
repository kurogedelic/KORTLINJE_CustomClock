#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <time.h>
#include <esp_crt_bundle.h>
#include <mqtt_client.h>
#include <esp_ota_ops.h>
#include <math.h>
#include <stdlib.h>
#include "MqttPrivate.h"
#include "BasicDisplay.h"
#include "ClockFaces.h"
#include "Games.h"
#include "Apps.h"
#include "MqttText.h"
#include "Patterns.h"
#include "PressStart2PClock.h"
#include "WebUi.h"
enum class PhysicalKey : uint8_t { None, Snooze, Brightness, Mode, Bell, Up, Down, Set, Alarm };

// Arduino core 2.0.17 otherwise marks an OTA image valid before setup().
// Leave it pending until Wi-Fi, OTA, HTTP and MQTT have survived a trial boot.
extern "C" bool verifyRollbackLater(void) { return true; }

// Pin assignment: Board.h
// Wi-Fi and OTA passwords are provisioned over USB serial and stored in NVS.
uint8_t brightness = 7;
constexpr uint32_t ANIMATION_MS = 92;
constexpr uint32_t LIFE_MS = 220;
constexpr uint32_t SNOW_MS = 125;
constexpr uint32_t RAIN_MS = 95;
constexpr uint32_t DITHER_MS = 130;
constexpr uint32_t SNAKE_MS = 170;
constexpr uint32_t PONG_MS = 95;
constexpr uint32_t APP_FRAME_MS = 130;
constexpr uint32_t BUTTON_SAMPLE_MS = 15;
constexpr uint32_t BUTTON_LONG_MS = 900;
constexpr uint32_t CLOCK_EFFECT_MS = 600;
constexpr uint32_t CLOCK_EFFECT_FRAME_MS = 50;
constexpr uint32_t BUTTON_PRESSED_MV = 2700;
constexpr uint32_t BUTTON_RELEASED_MV = 2800;
constexpr char TOPIC_TEXT[] = MQTT_TOPIC_PREFIX "/text";
constexpr char TOPIC_MODE[] = MQTT_TOPIC_PREFIX "/mode";
constexpr char TOPIC_FONT[] = MQTT_TOPIC_PREFIX "/font";
constexpr char TOPIC_EFFECT[] = MQTT_TOPIC_PREFIX "/effect";
constexpr char TOPIC_PREVIEW[] = MQTT_TOPIC_PREFIX "/preview";
constexpr char TOPIC_STATUS[] = MQTT_TOPIC_PREFIX "/status";
constexpr char TOPIC_EVENT[] = MQTT_TOPIC_PREFIX "/event";

Preferences settings;
String wifiSsid, wifiPassword, otaPassword, serialLine;
String weatherLat, weatherLon;
bool otaStarted = false;
bool webStarted = false;
bool otaTrial = false;
uint32_t otaTrialStarted = 0;
bool wasConnected = false;
enum class DisplayMode : uint8_t { Clock, Date, Seconds, Calendar, Life, Snow, Rain, Dither, Snake, Pong, Tempo, Pomodoro, Stopwatch, Countdown, Sky, Calibrate };
DisplayMode mode = DisplayMode::Clock;
uint32_t lastConnectAttempt = 0;
IkeaMatrix::Frame frame;
enum class Stage : uint8_t { Idle, MqttText };
Stage stage = Stage::Idle;
int scrollLeft = 0;
uint8_t backPressSamples = 0, backReleaseSamples = 0;
uint8_t topPressSamples = 0, topReleaseSamples = 0;
bool backHeld = false, topHeld = false;
bool topLongDone = false;
uint32_t topPressedAt = 0;
uint32_t lastBackMv = 0, lastTopMv = 0, backPressedMv = 0, topPressedMv = 0;
uint32_t backPressCount = 0, topPressCount = 0;
PhysicalKey activeBackKey = PhysicalKey::None, activeTopKey = PhysicalKey::None;
char calibrationLabel[6] = "PRESS";
String buttonModes[9];
uint32_t webUnlockedUntil = 0;
String webToken;
uint32_t lastAnimation = 0, lastLife = 0, lastButtonSample = 0;
uint32_t lastSnow = 0;
uint32_t lastRain = 0, lastDither = 0, lastGame = 0;
uint32_t lastAppFrame = 0;
uint32_t lifeGeneration = 0;
int lastClockMinute = -1;
int lastDateDay = -1;
int lastAuxSecond = -1;
ClockFaces::Font clockFont = ClockFaces::Font::Pixel;
ClockFaces::Effect clockEffect = ClockFaces::Effect::Off;
bool clockTransitionActive = false;
bool clockPreviewActive = false;
DisplayMode previewReturnMode = DisplayMode::Clock;
ClockFaces::Font transitionFont = ClockFaces::Font::Pixel;
ClockFaces::Effect transitionEffect = ClockFaces::Effect::Off;
uint32_t clockTransitionStarted = 0, lastClockEffectFrame = 0;
char shownClock[6] = "--:--", transitionFrom[6] = "--:--", transitionTo[6] = "--:--";
bool ntpRequested = false;
volatile bool mqttConnected = false;
volatile uint32_t mqttErrors = 0;
volatile uint32_t lastStatusPublish = 0;
esp_mqtt_client_handle_t mqttClient = nullptr;
WebServer web(80);
struct MqttCommand { uint8_t kind; uint16_t length; char text[MqttText::MAX_BYTES + 1]; };
QueueHandle_t mqttCommands = nullptr;
void mqttEventAck(const char *event);
void setMode(DisplayMode next);

bool timeValid() { return time(nullptr) > 1700000000; }

void formatClock(char text[6]) {
  memcpy(text, "--:--", 6);
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    snprintf(text, 6, "%02d:%02d", local.tm_hour, local.tm_min);
  }
}

void drawClock() {
  char text[6];
  formatClock(text);
  clockTransitionActive = false;
  memcpy(shownClock, text, sizeof shownClock);
  ClockFaces::draw(frame, text, clockFont);
  BasicDisplay::show(frame, brightness);
}

void startClockTransition(const char target[6]) {
  if (mode != DisplayMode::Clock || stage != Stage::Idle ||
      clockEffect == ClockFaces::Effect::Off || !strcmp(shownClock, target)) return;
  memcpy(transitionFrom, shownClock, sizeof transitionFrom);
  memcpy(transitionTo, target, sizeof transitionTo);
  transitionFont = clockFont;
  transitionEffect = clockEffect;
  clockPreviewActive = false;
  clockTransitionActive = true;
  clockTransitionStarted = millis();
  lastClockEffectFrame = 0;
}

void tickClockTransition() {
  if (!clockTransitionActive || mode != DisplayMode::Clock || stage != Stage::Idle) return;
  uint32_t now = millis();
  uint32_t elapsed = now - clockTransitionStarted;
  if (elapsed >= CLOCK_EFFECT_MS) {
    if (clockPreviewActive) {
      DisplayMode previous = previewReturnMode;
      clockPreviewActive = false;
      if (previous == DisplayMode::Clock) drawClock();
      else setMode(previous);
    } else drawClock();
    return;
  }
  if (lastClockEffectFrame && now - lastClockEffectFrame < CLOCK_EFFECT_FRAME_MS) return;
  lastClockEffectFrame = now;
  uint8_t phase = uint8_t((elapsed * 9) / CLOCK_EFFECT_MS);
  ClockFaces::transition(frame, transitionFrom, transitionTo,
                         transitionFont, transitionEffect, phase);
  BasicDisplay::show(frame, brightness);
}

bool previewClockTransition(ClockFaces::Font font, ClockFaces::Effect effect) {
  if (stage != Stage::Idle || !timeValid()) return false;
  DisplayMode previous = mode;
  if (mode != DisplayMode::Clock) setMode(DisplayMode::Clock);
  char target[6];
  memcpy(target, shownClock, sizeof target);
  if (target[4] >= '0' && target[4] <= '9') {
    target[4] = char('0' + (target[4] - '0' + 1) % 10);
    memcpy(transitionFrom, shownClock, sizeof transitionFrom);
    memcpy(transitionTo, target, sizeof transitionTo);
    transitionFont = font;
    transitionEffect = effect;
    previewReturnMode = previous;
    clockPreviewActive = true;
    clockTransitionActive = true;
    clockTransitionStarted = millis();
    lastClockEffectFrame = 0;
    return true;
  }
  return false;
}

void previewClockTransition() {
  previewClockTransition(clockFont, clockEffect);
}

void formatDate(char text[6]) {
  memcpy(text, "--/--", 6);
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    snprintf(text, 6, "%02d/%02d", local.tm_mon + 1, local.tm_mday);
  }
}

void formatSeconds(char text[6]) {
  memcpy(text, "--:--", 6);
  if (!timeValid()) return;
  time_t now = time(nullptr);
  struct tm local;
  localtime_r(&now, &local);
  snprintf(text, 6, "%02d:%02d", local.tm_min, local.tm_sec);
}

const char *modeName() {
  switch (mode) {
    case DisplayMode::Date: return "date";
    case DisplayMode::Seconds: return "seconds";
    case DisplayMode::Calendar: return "calendar";
    case DisplayMode::Life: return "life";
    case DisplayMode::Snow: return "snow";
    case DisplayMode::Rain: return "rain";
    case DisplayMode::Dither: return "dither";
    case DisplayMode::Snake: return "snake";
    case DisplayMode::Pong: return "pong";
    case DisplayMode::Tempo: return "tempo";
    case DisplayMode::Pomodoro: return "pomodoro";
    case DisplayMode::Stopwatch: return "stopwatch";
    case DisplayMode::Countdown: return "countdown";
    case DisplayMode::Sky: return "sky";
    case DisplayMode::Calibrate: return "calibrate";
    default: return "clock";
  }
}

void drawCurrent() {
  if (mode == DisplayMode::Calibrate) {
    PixelText::draw(frame, calibrationLabel);
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Life) {
    Patterns::drawLife(frame);
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Snow) {
    Patterns::Snow::draw(frame);
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Rain) {
    Patterns::Rain::draw(frame);
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Dither) {
    Patterns::Dither::draw(frame);
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Snake) {
    Games::Snake::draw(frame);
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Pong) {
    Games::Pong::draw(frame);
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Tempo) {
    Apps::Tempo::draw(frame, millis());
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Pomodoro) {
    Apps::Pomodoro::draw(frame, millis());
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Stopwatch) {
    Apps::Stopwatch::draw(frame, clockFont, millis());
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Countdown) {
    Apps::Countdown::draw(frame, clockFont, millis());
    BasicDisplay::show(frame, brightness);
  } else if (mode == DisplayMode::Sky) {
    Apps::Sky::draw(frame);
    BasicDisplay::show(frame, brightness);
  } else {
    char text[6];
    if (mode == DisplayMode::Date) {
      formatDate(text);
      ClockFaces::draw(frame, text, clockFont);
      BasicDisplay::show(frame, brightness);
    } else if (mode == DisplayMode::Seconds) {
      formatSeconds(text);
      ClockFaces::draw(frame, text, clockFont);
      BasicDisplay::show(frame, brightness);
    } else if (mode == DisplayMode::Calendar) {
      time_t now = time(nullptr);
      struct tm local;
      localtime_r(&now, &local);
      if (timeValid() && local.tm_sec % 8 < 3) {
        static const char *days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        PixelText::draw(frame, days[local.tm_wday]);
      } else {
        formatDate(text);
        ClockFaces::draw(frame, text, clockFont);
      }
      BasicDisplay::show(frame, brightness);
    } else drawClock();
  }
}

void setMode(DisplayMode next) {
  stage = Stage::Idle;
  clockTransitionActive = false;
  clockPreviewActive = false;
  mode = next;
  if (next == DisplayMode::Calibrate) {
    memcpy(calibrationLabel, "PRESS", sizeof calibrationLabel);
  } else if (next == DisplayMode::Life) {
    Patterns::seedLife();
    lifeGeneration = 0;
    lastLife = millis();
  } else if (next == DisplayMode::Snow) {
    Patterns::Snow::reset();
    lastSnow = millis();
  } else if (next == DisplayMode::Rain) {
    Patterns::Rain::reset();
    lastRain = millis();
  } else if (next == DisplayMode::Dither) {
    Patterns::Dither::reset();
    lastDither = millis();
  } else if (next == DisplayMode::Snake) {
    Games::Snake::reset();
    lastGame = millis();
  } else if (next == DisplayMode::Pong) {
    Games::Pong::reset();
    lastGame = millis();
  } else if (next == DisplayMode::Tempo) {
    Apps::Tempo::reset();
    lastAppFrame = millis();
  } else if (next == DisplayMode::Pomodoro) {
    Apps::Pomodoro::reset();
    lastAppFrame = millis();
  } else if (next == DisplayMode::Stopwatch) {
    Apps::Stopwatch::reset();
    lastAppFrame = millis();
  } else if (next == DisplayMode::Countdown) {
    Apps::Countdown::reset();
    lastAppFrame = millis();
  } else if (next == DisplayMode::Sky) {
    Apps::Sky::set(Apps::Sky::Condition::Sun);
    lastAppFrame = millis();
  } else if (next == DisplayMode::Clock) {
    lastClockMinute = -1;
  } else if (next == DisplayMode::Seconds || next == DisplayMode::Calendar) {
    lastAuxSecond = -1;
  } else {
    lastDateDay = -1;
  }
  drawCurrent();
}

bool parseMode(const char *name, DisplayMode &result) {
  if (strcmp(name, "clock") == 0) result = DisplayMode::Clock;
  else if (strcmp(name, "date") == 0) result = DisplayMode::Date;
  else if (strcmp(name, "seconds") == 0) result = DisplayMode::Seconds;
  else if (strcmp(name, "calendar") == 0) result = DisplayMode::Calendar;
  else if (strcmp(name, "life") == 0) result = DisplayMode::Life;
  else if (strcmp(name, "snow") == 0) result = DisplayMode::Snow;
  else if (strcmp(name, "rain") == 0) result = DisplayMode::Rain;
  else if (strcmp(name, "dither") == 0) result = DisplayMode::Dither;
  else if (strcmp(name, "snake") == 0) result = DisplayMode::Snake;
  else if (strcmp(name, "pong") == 0) result = DisplayMode::Pong;
  else if (strcmp(name, "tempo") == 0) result = DisplayMode::Tempo;
  else if (strcmp(name, "pomodoro") == 0) result = DisplayMode::Pomodoro;
  else if (strcmp(name, "stopwatch") == 0) result = DisplayMode::Stopwatch;
  else if (strcmp(name, "countdown") == 0) result = DisplayMode::Countdown;
  else if (strcmp(name, "sky") == 0) result = DisplayMode::Sky;
  else if (strcmp(name, "calibrate") == 0) result = DisplayMode::Calibrate;
  else return false;
  return true;
}

bool selectMode(const char *name) {
  DisplayMode selected;
  if (!parseMode(name, selected)) return false;
  setMode(selected);
  return true;
}

bool selectFont(const char *name) {
  ClockFaces::Font next;
  if (!ClockFaces::parseFont(name, next)) return false;
  clockFont = next;
  settings.putUChar("font", uint8_t(clockFont));
  clockTransitionActive = false;
  if (stage == Stage::Idle) drawCurrent();
  return true;
}

bool selectEffect(const char *name) {
  ClockFaces::Effect next;
  if (!ClockFaces::parseEffect(name, next)) return false;
  clockEffect = next;
  settings.putUChar("effect", uint8_t(clockEffect));
  clockTransitionActive = false;
  if (stage == Stage::Idle && mode == DisplayMode::Clock) drawClock();
  return true;
}

void cycleFont(int direction = 1) {
  clockFont = ClockFaces::Font((uint8_t(clockFont) + (direction > 0 ? 1 : 3)) % 4);
  settings.putUChar("font", uint8_t(clockFont));
  setMode(DisplayMode::Clock);
  mqttEventAck("font");
}

void cycleEffect() {
  clockEffect = ClockFaces::Effect((uint8_t(clockEffect) + 1) % 4);
  settings.putUChar("effect", uint8_t(clockEffect));
  setMode(DisplayMode::Clock);
  previewClockTransition();
  mqttEventAck("effect");
}

bool webUnlocked() {
  return webUnlockedUntil && int32_t(webUnlockedUntil - millis()) > 0;
}

bool nearMv(uint32_t value, uint32_t center) {
  return value >= center - 130 && value <= center + 130;
}

PhysicalKey decodeButton(bool top, uint32_t mv) {
  if (mv <= 150) return top ? PhysicalKey::Snooze : PhysicalKey::Set;
  if (nearMv(mv, 1648)) return top ? PhysicalKey::Bell : PhysicalKey::Alarm;
  if (nearMv(mv, top ? 2190 : 2172)) return top ? PhysicalKey::Brightness : PhysicalKey::Up;
  if (nearMv(mv, top ? 2470 : 2437)) return top ? PhysicalKey::Mode : PhysicalKey::Down;
  return PhysicalKey::None;
}

const char *keyName(PhysicalKey key) {
  switch (key) {
    case PhysicalKey::Snooze: return "snooze";
    case PhysicalKey::Brightness: return "brightness";
    case PhysicalKey::Mode: return "mode";
    case PhysicalKey::Bell: return "bell";
    case PhysicalKey::Up: return "up";
    case PhysicalKey::Down: return "down";
    case PhysicalKey::Set: return "set";
    case PhysicalKey::Alarm: return "alarm";
    default: return "none";
  }
}

const char *calibrationName(PhysicalKey key) {
  switch (key) {
    case PhysicalKey::Snooze: return "SNOOZ";
    case PhysicalKey::Brightness: return "BRITE";
    case PhysicalKey::Mode: return "MODE";
    case PhysicalKey::Bell: return "BELL";
    case PhysicalKey::Up: return "UP";
    case PhysicalKey::Down: return "DOWN";
    case PhysicalKey::Set: return "SET";
    case PhysicalKey::Alarm: return "ALARM";
    default: return "PRESS";
  }
}

PhysicalKey keyFromName(const String &name) {
  for (uint8_t value = 1; value <= 8; ++value) {
    PhysicalKey key = PhysicalKey(value);
    if (name == keyName(key)) return key;
  }
  return PhysicalKey::None;
}

void nextDisplayMode() {
  DisplayMode next = mode == DisplayMode::Clock ? DisplayMode::Date :
                     mode == DisplayMode::Date ? DisplayMode::Life :
                     mode == DisplayMode::Life ? DisplayMode::Snow :
                     mode == DisplayMode::Snow ? DisplayMode::Rain :
                     mode == DisplayMode::Rain ? DisplayMode::Dither : DisplayMode::Clock;
  setMode(next);
  mqttEventAck(modeName());
}

void nextBrightness() {
  brightness = brightness == 7 ? 5 : brightness == 5 ? 3 : brightness == 3 ? 1 : 7;
  settings.putUChar("brightness", brightness);
  BasicDisplay::show(frame, brightness);
  mqttEventAck("brightness");
}

void buttonAction(PhysicalKey key) {
  if (key == PhysicalKey::None) return;
  if (mode == DisplayMode::Calibrate) {
    snprintf(calibrationLabel, sizeof calibrationLabel, "%s", calibrationName(key));
    drawCurrent();
    if (key == PhysicalKey::Set) webUnlockedUntil = millis() + 60000;
    return;
  }
  if (key == PhysicalKey::Set) {
    webUnlockedUntil = millis() + 60000;
    mqttEventAck("web_unlock");
  }
  const String &assigned = buttonModes[uint8_t(key)];
  if (assigned.length() && selectMode(assigned.c_str())) {
    mqttEventAck(modeName());
    return;
  }
  if (key == PhysicalKey::Set) return;
  if (key == PhysicalKey::Up) { cycleFont(1); return; }
  if (key == PhysicalKey::Down) { cycleFont(-1); return; }
  if (mode == DisplayMode::Snake) {
    if (key == PhysicalKey::Snooze) Games::Snake::control("up");
    else if (key == PhysicalKey::Brightness) Games::Snake::control("left");
    else if (key == PhysicalKey::Mode) Games::Snake::control("down");
    else if (key == PhysicalKey::Bell) Games::Snake::control("right");
    else if (key == PhysicalKey::Alarm) Games::Snake::reset();
    drawCurrent(); return;
  }
  if (mode == DisplayMode::Pong) {
    if (key == PhysicalKey::Snooze) Games::Pong::control("up");
    else if (key == PhysicalKey::Brightness) Games::Pong::control("down");
    else if (key == PhysicalKey::Mode) Games::Pong::toggle();
    else if (key == PhysicalKey::Bell) Games::Pong::reset();
    drawCurrent(); return;
  }
  if (mode == DisplayMode::Tempo) {
    if (key == PhysicalKey::Snooze) Apps::Tempo::tap(millis());
    else if (key == PhysicalKey::Brightness) Apps::Tempo::nextMeter();
    else if (key == PhysicalKey::Bell) Apps::Tempo::reset();
    else if (key == PhysicalKey::Mode) { setMode(DisplayMode::Clock); return; }
    drawCurrent(); return;
  }
  if (mode == DisplayMode::Pomodoro) {
    if (key == PhysicalKey::Snooze) Apps::Pomodoro::toggle();
    else if (key == PhysicalKey::Brightness) Apps::Pomodoro::reset();
    else if (key == PhysicalKey::Bell) Apps::Pomodoro::skip();
    else if (key == PhysicalKey::Mode) { setMode(DisplayMode::Clock); return; }
    drawCurrent(); return;
  }
  if (mode == DisplayMode::Stopwatch) {
    if (key == PhysicalKey::Snooze) Apps::Stopwatch::toggle();
    else if (key == PhysicalKey::Brightness) Apps::Stopwatch::reset();
    else if (key == PhysicalKey::Bell) Apps::Stopwatch::lap();
    else if (key == PhysicalKey::Mode) { setMode(DisplayMode::Clock); return; }
    drawCurrent(); return;
  }
  if (mode == DisplayMode::Countdown) {
    if (key == PhysicalKey::Snooze) Apps::Countdown::toggle();
    else if (key == PhysicalKey::Brightness) Apps::Countdown::reset();
    else if (key == PhysicalKey::Bell) Apps::Countdown::configure(60);
    else if (key == PhysicalKey::Mode) { setMode(DisplayMode::Clock); return; }
    drawCurrent(); return;
  }
  if (mode == DisplayMode::Sky) {
    if (key == PhysicalKey::Snooze) Apps::Sky::set(Apps::Sky::Condition::Sun);
    else if (key == PhysicalKey::Brightness) Apps::Sky::set(Apps::Sky::Condition::Cloud);
    else if (key == PhysicalKey::Mode) Apps::Sky::set(Apps::Sky::Condition::Rain);
    else if (key == PhysicalKey::Bell) Apps::Sky::set(Apps::Sky::Condition::Snow);
    drawCurrent(); return;
  }
  switch (key) {
    case PhysicalKey::Snooze: setMode(DisplayMode::Clock); break;
    case PhysicalKey::Brightness: nextBrightness(); break;
    case PhysicalKey::Mode: nextDisplayMode(); break;
    case PhysicalKey::Bell: cycleEffect(); break;
    case PhysicalKey::Up: cycleFont(1); break;
    case PhysicalKey::Down: cycleFont(-1); break;
    case PhysicalKey::Set: break;
    case PhysicalKey::Alarm: setMode(DisplayMode::Calibrate); break;
    default: break;
  }
}

bool topicIs(esp_mqtt_event_handle_t event, const char *topic) {
  size_t length = strlen(topic);
  return event->topic && event->topic_len == int(length) &&
         memcmp(event->topic, topic, length) == 0;
}

esp_err_t mqttEvent(esp_mqtt_event_handle_t event) {
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      mqttConnected = true;
      esp_mqtt_client_subscribe(event->client, TOPIC_TEXT, 1);
      esp_mqtt_client_subscribe(event->client, TOPIC_MODE, 1);
      esp_mqtt_client_subscribe(event->client, TOPIC_FONT, 1);
      esp_mqtt_client_subscribe(event->client, TOPIC_EFFECT, 1);
      esp_mqtt_client_subscribe(event->client, TOPIC_PREVIEW, 1);
      if (esp_mqtt_client_publish(event->client, TOPIC_STATUS, "online", 0, 1, 1) >= 0)
        lastStatusPublish = millis();
      else
        ++mqttErrors;
      break;
    case MQTT_EVENT_DISCONNECTED:
      mqttConnected = false;
      break;
    case MQTT_EVENT_ERROR:
      ++mqttErrors;
      break;
    case MQTT_EVENT_DATA: {
      if (!mqttCommands || event->current_data_offset != 0 ||
          event->data_len != event->total_data_len) break;
      MqttCommand command{};
      if (topicIs(event, TOPIC_TEXT) && event->data_len > 0 &&
          event->data_len <= MqttText::MAX_BYTES) {
        command.kind = 1;
        command.length = event->data_len;
        memcpy(command.text, event->data, event->data_len);
        command.text[event->data_len] = '\0';
      } else if (topicIs(event, TOPIC_MODE) && event->data_len > 0 &&
                 event->data_len <= 9) {
        command.kind = 3;
        command.length = event->data_len;
        memcpy(command.text, event->data, event->data_len);
        command.text[event->data_len] = '\0';
      } else if ((topicIs(event, TOPIC_FONT) || topicIs(event, TOPIC_EFFECT)) &&
                 event->data_len > 0 && event->data_len <= 8) {
        command.kind = topicIs(event, TOPIC_FONT) ? 4 : 5;
        command.length = event->data_len;
        memcpy(command.text, event->data, event->data_len);
        command.text[event->data_len] = '\0';
      } else if (topicIs(event, TOPIC_PREVIEW) && event->data_len == 5 &&
                 memcmp(event->data, "clock", 5) == 0) {
        command.kind = 6;
      }
      if (command.kind) xQueueOverwrite(mqttCommands, &command);
      break;
    }
    default:
      break;
  }
  return ESP_OK;
}

void startMqtt() {
  if (!MQTT_URI[0] || mqttClient || WiFi.status() != WL_CONNECTED || !timeValid()) return;
  esp_mqtt_client_config_t config{};
  config.uri = MQTT_URI;
  config.username = MQTT_USERNAME;
  config.password = MQTT_PASSWORD;
  config.client_id = MQTT_CLIENT_ID;
  config.crt_bundle_attach = esp_crt_bundle_attach;
  config.lwt_topic = TOPIC_STATUS;
  config.lwt_msg = "offline";
  config.lwt_qos = 1;
  config.lwt_retain = 1;
  config.keepalive = 30;
  config.buffer_size = 1024;
  config.event_handle = mqttEvent;
  mqttClient = esp_mqtt_client_init(&config);
  if (mqttClient && esp_mqtt_client_start(mqttClient) == ESP_OK) {
    Serial.println("MQTT WSS started");
  } else {
    if (mqttClient) esp_mqtt_client_destroy(mqttClient);
    mqttClient = nullptr;
    ++mqttErrors;
  }
}

void mqttEventAck(const char *event) {
  if (mqttConnected && mqttClient)
    esp_mqtt_client_publish(mqttClient, TOPIC_EVENT, event, 0, 1, 0);
}

String webSettingsJson() {
  char clockText[6];
  char dateText[6];
  formatClock(clockText);
  formatDate(dateText);
  String result = "{\"version\":\"1.25\",\"clock\":\"";
  result += clockText;
  result += "\",\"date\":\"";
  result += dateText;
  result += "\",\"mode\":\"";
  result += modeName();
  result += "\",\"font\":\"";
  result += ClockFaces::name(clockFont);
  result += "\",\"effect\":\"";
  result += ClockFaces::name(clockEffect);
  result += "\",\"brightness\":" + String(brightness);
  result += ",\"invert\":";
  result += IkeaMatrix::invertPixels ? "true" : "false";
  result += ",\"unlocked\":";
  result += webUnlocked() ? "true" : "false";
  result += ",\"remaining_s\":" + String(webUnlocked() ? (webUnlockedUntil - millis() + 999) / 1000 : 0);
  result += ",\"button_map\":{";
  for (uint8_t value = 1; value <= 8; ++value) {
    if (value > 1) result += ",";
    PhysicalKey key = PhysicalKey(value);
    result += "\"";
    result += keyName(key);
    result += "\":\"";
    result += (buttonModes[value].isEmpty() ? String("default") : buttonModes[value]);
    result += "\"";
  }
  result += "},\"weather_lat\":\"" + weatherLat +
            "\",\"weather_lon\":\"" + weatherLon +
            "\",\"token\":\"" + webToken + "\"}";
  return result;
}

bool parseCoordinate(const String &text, float limit, String &normalised) {
  if (text.isEmpty() || text.length() > 16) return false;
  char *end = nullptr;
  float value = strtof(text.c_str(), &end);
  if (end == text.c_str() || *end != '\0' || !isfinite(value) ||
      value < -limit || value > limit) return false;
  normalised = String(value, 5);
  return true;
}

bool permitWebChange() {
  if (!webUnlocked() || web.arg("token") != webToken) {
    web.send(423, "application/json", "{\"error\":\"背面のSETボタンを押してください\"}");
    return false;
  }
  return true;
}

bool permitWebPreview() {
  if (webToken.isEmpty() || web.arg("token") != webToken) {
    web.send(403, "application/json", "{\"error\":\"ページを再読み込みしてください\"}");
    return false;
  }
  return true;
}

void startWebStatus() {
  if (webStarted || WiFi.status() != WL_CONNECTED) return;
  web.on("/status", HTTP_GET, []() {
    char clockText[6];
    char dateText[6];
    formatClock(clockText);
    formatDate(dateText);
    String result = "{\"version\":\"1.25\",\"clock\":\"";
    result += clockText;
    result += "\",\"date\":\"";
    result += dateText;
    result += "\",\"mode\":\"";
    result += modeName();
    result += "\",\"calibration_label\":\"";
    result += calibrationLabel;
    result += "\",\"font\":\"";
    result += ClockFaces::name(clockFont);
    result += "\",\"effect\":\"";
    result += ClockFaces::name(clockEffect);
    result += "\",\"brightness\":" + String(brightness);
    result += ",\"invert\":";
    result += IkeaMatrix::invertPixels ? "true" : "false";
    result += ",\"web_unlocked\":";
    result += webUnlocked() ? "true" : "false";
    result += ",\"wifi\":";
    result += WiFi.status() == WL_CONNECTED ? "true" : "false";
    result += ",\"time\":";
    result += timeValid() ? "true" : "false";
    result += ",\"mqtt\":";
    result += mqttConnected ? "true" : "false";
    result += ",\"mqtt_errors\":" + String(mqttErrors);
    result += ",\"rssi\":" + String(WiFi.RSSI());
    result += ",\"show\":" + String(unsigned(stage));
    result += ",\"life_generation\":" + String(lifeGeneration);
    result += ",\"snake_score\":" + String(Games::Snake::length - 4);
    result += ",\"snake_over\":";
    result += Games::Snake::over ? "true" : "false";
    result += ",\"pong_player\":" + String(Games::Pong::playerScore);
    result += ",\"pong_cpu\":" + String(Games::Pong::cpuScore);
    result += ",\"pong_paddle\":" + String(Games::Pong::player);
    result += ",\"pong_paused\":";
    result += Games::Pong::paused ? "true" : "false";
    result += ",\"tempo_bpm\":" + String(Apps::Tempo::bpm);
    result += ",\"tempo_taps\":" + String(Apps::Tempo::count + (Apps::Tempo::lastTap ? 1 : 0));
    result += ",\"tempo_meter\":" + String(Apps::Tempo::meter);
    result += ",\"pomodoro_remaining_s\":" + String((Apps::Pomodoro::remainingMs + 999) / 1000);
    result += ",\"pomodoro_work\":";
    result += Apps::Pomodoro::work ? "true" : "false";
    result += ",\"pomodoro_running\":";
    result += Apps::Pomodoro::running ? "true" : "false";
    result += ",\"pomodoro_cycles\":" + String(Apps::Pomodoro::cycles);
    result += ",\"stopwatch_ms\":" + String(Apps::Stopwatch::elapsedMs);
    result += ",\"stopwatch_running\":";
    result += Apps::Stopwatch::running ? "true" : "false";
    result += ",\"countdown_remaining_s\":" + String((Apps::Countdown::remainingMs + 999) / 1000);
    result += ",\"countdown_preset_s\":" + String(Apps::Countdown::presetSeconds);
    result += ",\"countdown_running\":";
    result += Apps::Countdown::running ? "true" : "false";
    result += ",\"countdown_finished\":";
    result += Apps::Countdown::finished ? "true" : "false";
    result += ",\"sky_condition\":\"";
    result += Apps::Sky::name();
    result += "\"";
    result += "}";
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", result);
  });
  web.on("/buttons", HTTP_GET, []() {
    String result = "{\"back_mv\":" + String(lastBackMv);
    result += ",\"top_mv\":" + String(lastTopMv);
    result += ",\"back_last_press_mv\":" + String(backPressedMv);
    result += ",\"top_last_press_mv\":" + String(topPressedMv);
    result += ",\"back_count\":" + String(backPressCount);
    result += ",\"top_count\":" + String(topPressCount);
    result += ",\"back_held\":";
    result += backHeld ? "true" : "false";
    result += ",\"top_held\":";
    result += topHeld ? "true" : "false";
    result += ",\"back_key\":\"";
    result += keyName(activeBackKey);
    result += "\",\"top_key\":\"";
    result += keyName(activeTopKey);
    result += "\"";
    result += "}";
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", result);
  });
  web.on("/", HTTP_GET, []() {
    web.sendHeader("Cache-Control", "no-store");
    web.sendHeader("X-Frame-Options", "DENY");
    web.send_P(200, "text/html; charset=utf-8", KORTLINJE_WEB_UI);
  });
  web.on("/api/settings", HTTP_GET, []() {
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", webSettingsJson());
  });
  web.on("/api/button-map", HTTP_POST, []() {
    if (!permitWebChange()) return;
    PhysicalKey key = keyFromName(web.arg("key"));
    String target = web.arg("mode");
    DisplayMode parsed;
    if (key == PhysicalKey::None ||
        (target != "default" && !parseMode(target.c_str(), parsed))) {
      web.send(400, "application/json", "{\"error\":\"Invalid button assignment\"}");
      return;
    }
    const uint8_t index = uint8_t(key);
    buttonModes[index] = target == "default" ? "" : target;
    String settingKey = "btn" + String(index);
    settings.putString(settingKey.c_str(), buttonModes[index]);
    web.send(200, "application/json", webSettingsJson());
  });
  web.on("/api/weather-config", HTTP_POST, []() {
    if (!permitWebChange()) return;
    String lat = web.arg("lat"), lon = web.arg("lon");
    String nextLat, nextLon;
    if (lat.isEmpty() && lon.isEmpty()) {
      nextLat = "";
      nextLon = "";
    } else if (!parseCoordinate(lat, 90.0f, nextLat) ||
               !parseCoordinate(lon, 180.0f, nextLon)) {
      web.send(400, "application/json", "{\"error\":\"Invalid coordinates\"}");
      return;
    }
    weatherLat = nextLat;
    weatherLon = nextLon;
    settings.putString("weather_lat", weatherLat);
    settings.putString("weather_lon", weatherLon);
    web.send(200, "application/json", webSettingsJson());
  });
  web.on("/api/frame", HTTP_GET, []() {
    String result;
    result.reserve(410);
    result = "{\"mode\":\"";
    result += modeName();
    result += "\",\"bits\":\"";
    for (int y = 0; y < IkeaMatrix::HEIGHT; ++y)
      for (int x = 0; x < IkeaMatrix::WIDTH; ++x)
        result += char('0' + (ClockFaces::get(frame, x, y) != IkeaMatrix::invertPixels));
    result += "\"}";
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", result);
  });
  web.on("/api/mode", HTTP_POST, []() {
    if (!permitWebPreview()) return;
    if (!selectMode(web.arg("mode").c_str())) {
      web.send(400, "application/json", "{\"error\":\"表示モードが正しくありません\"}");
      return;
    }
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.on("/api/game", HTTP_POST, []() {
    if (!permitWebPreview()) return;
    String action = web.arg("action");
    if (action != "up" && action != "down" && action != "left" &&
        action != "right" && action != "restart" && action != "paddle") {
      web.send(400, "application/json", "{\"error\":\"操作が正しくありません\"}");
      return;
    }
    if (mode == DisplayMode::Snake && action != "paddle") Games::Snake::control(action);
    else if (mode == DisplayMode::Pong) {
      if (action == "paddle") {
        String value = web.arg("value");
        if (value.length() != 1 || value[0] < '0' || value[0] > '6') {
          web.send(400, "application/json", "{\"error\":\"パドル位置が正しくありません\"}");
          return;
        }
        Games::Pong::setPaddle(uint8_t(value[0] - '0'));
      } else Games::Pong::control(action);
    }
    else {
      web.send(409, "application/json", "{\"error\":\"ゲームを選択してください\"}");
      return;
    }
    drawCurrent();
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.on("/api/app", HTTP_POST, []() {
    if (!permitWebPreview()) return;
    String action = web.arg("action");
    bool ok = true;
    if (mode == DisplayMode::Tempo) {
      if (action == "tap") Apps::Tempo::tap(millis());
      else if (action == "reset") Apps::Tempo::reset();
      else if (action == "meter") Apps::Tempo::nextMeter();
      else ok = false;
    } else if (mode == DisplayMode::Pomodoro) {
      if (action == "toggle") Apps::Pomodoro::toggle();
      else if (action == "reset") Apps::Pomodoro::reset();
      else if (action == "skip") Apps::Pomodoro::skip();
      else if (action == "demo") Apps::Pomodoro::demo();
      else if (action == "configure") {
        String workText = web.arg("work"), breakText = web.arg("break");
        int workValue = workText.toInt(), breakValue = breakText.toInt();
        if (workValue < 1 || workValue > 90 || breakValue < 1 || breakValue > 90)
          ok = false;
        else Apps::Pomodoro::configure(uint8_t(workValue), uint8_t(breakValue));
      } else ok = false;
    } else if (mode == DisplayMode::Stopwatch) {
      if (action == "toggle") Apps::Stopwatch::toggle();
      else if (action == "reset") Apps::Stopwatch::reset();
      else if (action == "lap") Apps::Stopwatch::lap();
      else ok = false;
    } else if (mode == DisplayMode::Countdown) {
      if (action == "toggle") Apps::Countdown::toggle();
      else if (action == "reset") Apps::Countdown::reset();
      else if (action == "configure") {
        String secondsText = web.arg("seconds");
        if (secondsText.isEmpty() || secondsText.length() > 4) ok = false;
        else {
          for (size_t i = 0; i < secondsText.length(); ++i)
            if (secondsText[i] < '0' || secondsText[i] > '9') ok = false;
          if (ok) ok = Apps::Countdown::configure(uint16_t(secondsText.toInt()));
        }
      } else ok = false;
    } else if (mode == DisplayMode::Sky) {
      if (action == "set") ok = Apps::Sky::parse(web.arg("condition"));
      else ok = false;
    } else {
      web.send(409, "application/json", "{\"error\":\"アプリを選んでください\"}");
      return;
    }
    if (!ok) {
      web.send(400, "application/json", "{\"error\":\"操作が正しくありません\"}");
      return;
    }
    drawCurrent();
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.on("/api/style-preview", HTTP_POST, []() {
    if (!permitWebPreview()) return;
    ClockFaces::Font font;
    ClockFaces::Effect effect;
    String chosenBrightness = web.arg("brightness"), chosenInvert = web.arg("invert");
    if (!ClockFaces::parseFont(web.arg("font").c_str(), font) ||
        !ClockFaces::parseEffect(web.arg("effect").c_str(), effect) ||
        chosenBrightness.length() != 1 || chosenBrightness[0] < '0' ||
        chosenBrightness[0] > '7' ||
        (chosenInvert != "0" && chosenInvert != "1")) {
      web.send(400, "application/json", "{\"error\":\"プレビュー値が正しくありません\"}");
      return;
    }
    clockFont = font;
    clockEffect = effect;
    brightness = uint8_t(chosenBrightness[0] - '0');
    IkeaMatrix::invertPixels = chosenInvert == "1";
    drawCurrent();
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.on("/api/settings", HTTP_POST, []() {
    if (!permitWebChange()) return;
    String chosenMode = web.arg("mode"), chosenFont = web.arg("font");
    String chosenEffect = web.arg("effect"), chosenBrightness = web.arg("brightness");
    String chosenInvert = web.arg("invert");
    ClockFaces::Font nextFont;
    ClockFaces::Effect nextEffect;
    bool modeOK = chosenMode == "clock" || chosenMode == "date" ||
                  chosenMode == "seconds" || chosenMode == "calendar" ||
                  chosenMode == "life" || chosenMode == "snow" ||
                  chosenMode == "rain" || chosenMode == "dither";
    bool brightnessOK = chosenBrightness.length() == 1 &&
                        chosenBrightness[0] >= '0' && chosenBrightness[0] <= '7';
    if (!modeOK || !ClockFaces::parseFont(chosenFont.c_str(), nextFont) ||
        !ClockFaces::parseEffect(chosenEffect.c_str(), nextEffect) || !brightnessOK ||
        (chosenInvert != "0" && chosenInvert != "1")) {
      web.send(400, "application/json", "{\"error\":\"設定値が正しくありません\"}");
      return;
    }
    clockFont = nextFont;
    clockEffect = nextEffect;
    brightness = uint8_t(chosenBrightness[0] - '0');
    IkeaMatrix::invertPixels = chosenInvert == "1";
    settings.putUChar("font", uint8_t(clockFont));
    settings.putUChar("effect", uint8_t(clockEffect));
    settings.putUChar("brightness", brightness);
    settings.putBool("invert", IkeaMatrix::invertPixels);
    settings.putString("mode", chosenMode);
    selectMode(chosenMode.c_str());
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.on("/api/preview", HTTP_POST, []() {
    if (web.arg("token") != webToken || webToken.isEmpty()) {
      web.send(403, "application/json", "{\"error\":\"ページを再読み込みしてください\"}");
      return;
    }
    ClockFaces::Font font;
    ClockFaces::Effect effect;
    if (!ClockFaces::parseFont(web.arg("font").c_str(), font) ||
        !ClockFaces::parseEffect(web.arg("effect").c_str(), effect)) {
      web.send(400, "application/json", "{\"error\":\"演出の指定が正しくありません\"}");
      return;
    }
    if (!previewClockTransition(font, effect)) {
      web.send(409, "application/json", "{\"error\":\"時計がほかの表示を再生中です\"}");
      return;
    }
    web.send(200, "application/json", "{\"ok\":true}");
  });
  web.begin();
  webStarted = true;
}

void checkTrialBoot() {
  if (!otaTrial) return;
  const uint32_t elapsed = millis() - otaTrialStarted;
  if (elapsed >= 10000 && WiFi.status() == WL_CONNECTED && otaStarted &&
      webStarted && (!MQTT_URI[0] || mqttConnected)) {
    esp_err_t result = esp_ota_mark_app_valid_cancel_rollback();
    if (result == ESP_OK) {
      otaTrial = false;
      Serial.println("OTA trial passed; image confirmed");
    } else {
      Serial.printf("OTA confirm failed: %d\n", int(result));
    }
  } else if (elapsed >= 90000) {
    Serial.println("OTA trial failed; rolling back");
    Serial.flush();
    esp_ota_mark_app_invalid_rollback_and_reboot();
    ESP.restart();
  }
}

void startMqttText(const char *text, size_t length) {
  if (!MqttText::prepare(text, length)) {
    mqttEventAck("unsupported_text");
    return;
  }
  clockTransitionActive = false;
  stage = Stage::MqttText;
  scrollLeft = 41;
  lastAnimation = millis();
  MqttText::draw(frame, scrollLeft);
  BasicDisplay::show(frame, brightness);
  mqttEventAck("text");
}

void pollButtons() {
  uint32_t now = millis();
  if (now < 1800 || now - lastButtonSample < BUTTON_SAMPLE_MS) return;
  lastButtonSample = now;
  uint32_t back = analogReadMilliVolts(BACK_BUTTON_PIN);
  uint32_t top = analogReadMilliVolts(TOP_BUTTON_PIN);
  lastBackMv = back;
  lastTopMv = top;
  if (back < BUTTON_PRESSED_MV) {
    backReleaseSamples = 0;
    if (backPressSamples < 3) ++backPressSamples;
    if (!backHeld && backPressSamples == 3) {
      backHeld = true;
      backPressedMv = back;
      activeBackKey = decodeButton(false, back);
      ++backPressCount;
    }
  } else if (back > BUTTON_RELEASED_MV) {
    backPressSamples = 0;
    if (backReleaseSamples < 3) ++backReleaseSamples;
    if (backHeld && backReleaseSamples == 3) {
      backHeld = false;
      buttonAction(activeBackKey);
    }
  }
  if (top < BUTTON_PRESSED_MV) {
    topReleaseSamples = 0;
    if (topPressSamples < 3) ++topPressSamples;
    if (!topHeld && topPressSamples == 3) {
      topHeld = true;
      topLongDone = false;
      topPressedAt = now;
      topPressedMv = top;
      activeTopKey = decodeButton(true, top);
      ++topPressCount;
    }
  } else if (top > BUTTON_RELEASED_MV) {
    topPressSamples = 0;
    if (topReleaseSamples < 3) ++topReleaseSamples;
    if (topHeld && topReleaseSamples == 3) {
      topHeld = false;
      if (!topLongDone) buttonAction(activeTopKey);
    }
  }
  if (topHeld && !topLongDone && now - topPressedAt >= BUTTON_LONG_MS) {
    if (activeTopKey == PhysicalKey::Mode) {
      topLongDone = true;
      setMode(DisplayMode::Clock);
    } else if (activeTopKey == PhysicalKey::Bell && mode == DisplayMode::Clock) {
      topLongDone = true;
      selectEffect("off");
      setMode(DisplayMode::Clock);
    }
  }
}

void tickShow() {
  if (stage == Stage::Idle) return;
  uint32_t now = millis();
  if (now - lastAnimation < ANIMATION_MS) return;
  lastAnimation = now;
  switch (stage) {
    case Stage::MqttText:
      if (--scrollLeft < -MqttText::length) {
        stage = Stage::Idle;
        drawCurrent();
        return;
      }
      MqttText::draw(frame, scrollLeft);
      break;
    case Stage::Idle:
      return;
  }
  BasicDisplay::show(frame, brightness);
}

void connectWifi() {
  if (wifiSsid.isEmpty()) return;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setHostname("kortlinje-c3");
  WiFi.setAutoReconnect(true);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  lastConnectAttempt = millis();
  Serial.println("Wi-Fi connection started");
}

void startOta() {
  if (otaStarted || WiFi.status() != WL_CONNECTED || otaPassword.length() < 12) return;
  ArduinoOTA.setHostname("kortlinje-c3");
  ArduinoOTA.setPassword(otaPassword.c_str());
  ArduinoOTA.setTimeout(10000);
  ArduinoOTA.onStart([]() { Serial.println("OTA started"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA completed"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error=%u\n", unsigned(error));
  });
  ArduinoOTA.begin();
  MDNS.addServiceTxt("arduino", "tcp", "version", "1.25");
  otaStarted = true;
  Serial.print("OTA ready at ");
  Serial.println(WiFi.localIP());
}

void printStatus() {
  Serial.printf("status version=1.25 wifi=%u ota=%u trial=%u ip=%s rssi=%ld mode=%s font=%s effect=%s brightness=%u show=%u time=%u mqtt=%u errors=%lu\n",
                WiFi.status() == WL_CONNECTED, otaStarted,
                otaTrial,
                WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "-",
                static_cast<long>(WiFi.RSSI()), modeName(),
                ClockFaces::name(clockFont), ClockFaces::name(clockEffect), brightness, unsigned(stage),
                timeValid(), mqttConnected,
                static_cast<unsigned long>(mqttErrors));
}

void printButtons() {
  Serial.printf("buttons back_gpio0=%lu mV top_gpio1=%lu mV back_key=%s top_key=%s\n",
                static_cast<unsigned long>(analogReadMilliVolts(BACK_BUTTON_PIN)),
                static_cast<unsigned long>(analogReadMilliVolts(TOP_BUTTON_PIN)),
                keyName(activeBackKey), keyName(activeTopKey));
}

void scanConfiguredWifi() {
  if (wifiSsid.isEmpty()) {
    Serial.println("wifi-scan no SSID configured");
    return;
  }
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  delay(300);
  const int count = WiFi.scanNetworks(false, true);
  if (count < 0) {
    Serial.printf("wifi-scan failed=%d\n", count);
    connectWifi();
    return;
  }
  int matches = 0;
  for (int i = 0; i < count; ++i) {
    if (WiFi.SSID(i) != wifiSsid) continue;
    ++matches;
    Serial.printf("wifi-scan match channel=%d rssi=%ld auth=%d\n",
                  WiFi.channel(i), static_cast<long>(WiFi.RSSI(i)),
                  int(WiFi.encryptionType(i)));
  }
  Serial.printf("wifi-scan networks=%d matches=%d status=%d\n",
                count, matches, int(WiFi.status()));
  WiFi.scanDelete();
  connectWifi();
}

void command(const String &line) {
  if (line.startsWith("wifi ")) {
    int divider = line.indexOf('\t', 5);
    if (divider < 0) { Serial.println("wifi format: wifi SSID<TAB>PASSWORD"); return; }
    String ssid = line.substring(5, divider);
    String password = line.substring(divider + 1);
    if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 63) {
      Serial.println("wifi invalid length"); return;
    }
    wifiSsid = ssid;
    wifiPassword = password;
    settings.putString("wifi_ssid", wifiSsid);
    settings.putString("wifi_pass", wifiPassword);
    Serial.println("Wi-Fi settings saved");
    WiFi.disconnect(false, false);
    connectWifi();
    return;
  }
  if (line.startsWith("ota ")) {
    String password = line.substring(4);
    if (password.length() < 12 || password.length() > 64) {
      Serial.println("OTA password must be 12-64 characters"); return;
    }
    otaPassword = password;
    settings.putString("ota_key", otaPassword);
    Serial.println("OTA password saved; reboot to activate it");
    return;
  }
  if (line == "status") { printStatus(); return; }
  if (line == "wifi-scan") { scanConfiguredWifi(); return; }
  if (line == "buttons") { printButtons(); return; }
  if (line == "reboot") { ESP.restart(); return; }
  if (line.startsWith("font ") && selectFont(line.c_str() + 5)) return;
  if (line.startsWith("effect ") && selectEffect(line.c_str() + 7)) return;
  if (line.startsWith("brightness ") && line.length() == 12 &&
      line[11] >= '0' && line[11] <= '7') {
    brightness = uint8_t(line[11] - '0');
    settings.putUChar("brightness", brightness);
    BasicDisplay::show(frame, brightness);
    return;
  }
  if (line == "preview") { previewClockTransition(); return; }
  if (selectMode(line.c_str())) {
    Serial.printf("display mode=%s\n", modeName());
    return;
  }
  Serial.println("commands: wifi, ota, status, buttons, reboot, clock, date, life, snow, rain, dither, snake, pong, calibrate, font, effect, brightness, preview");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("KORTLINJE_C3_OTA v1.25");
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    Serial.printf("Wi-Fi disconnected reason=%u\n",
                  unsigned(info.wifi_sta_disconnected.reason));
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t otaState;
  otaTrial = running && esp_ota_get_state_partition(running, &otaState) == ESP_OK &&
             otaState == ESP_OTA_IMG_PENDING_VERIFY;
  otaTrialStarted = millis();
  if (otaTrial) Serial.println("OTA trial boot; waiting for network self-test");
  settings.begin("kortlinje", false);
  wifiSsid = settings.getString("wifi_ssid", "");
  wifiPassword = settings.getString("wifi_pass", "");
  otaPassword = settings.getString("ota_key", "");
  weatherLat = settings.getString("weather_lat", "");
  weatherLon = settings.getString("weather_lon", "");
  for (uint8_t index = 1; index <= 8; ++index) {
    String settingKey = "btn" + String(index);
    buttonModes[index] = settings.getString(settingKey.c_str(), "");
    DisplayMode parsed;
    if (!buttonModes[index].isEmpty() &&
        !parseMode(buttonModes[index].c_str(), parsed))
      buttonModes[index] = "";
  }
  uint8_t storedFont = settings.getUChar("font", 0);
  uint8_t storedEffect = settings.getUChar("effect", 0);
  uint8_t storedBrightness = settings.getUChar("brightness", 7);
  String savedMode = settings.getString("mode", "clock");
  clockFont = ClockFaces::Font(storedFont < 4 ? storedFont : 0);
  clockEffect = ClockFaces::Effect(storedEffect < 4 ? storedEffect : 0);
  brightness = storedBrightness <= 7 ? storedBrightness : 7;
  IkeaMatrix::invertPixels = settings.getBool("invert", false);
  webToken = String(esp_random(), HEX) + String(esp_random(), HEX);
  analogReadResolution(12);
  pinMode(BACK_BUTTON_PIN, INPUT);
  pinMode(TOP_BUTTON_PIN, INPUT);
  analogSetPinAttenuation(BACK_BUTTON_PIN, ADC_11db);
  analogSetPinAttenuation(TOP_BUTTON_PIN, ADC_11db);
  randomSeed(esp_random());
  IkeaMatrix::rotate180 = true;
  mqttCommands = xQueueCreate(1, sizeof(MqttCommand));
  BasicDisplay::begin();
  if (savedMode == "clock" || savedMode == "date" || savedMode == "seconds" ||
      savedMode == "calendar" || savedMode == "life" ||
      savedMode == "snow" || savedMode == "rain" || savedMode == "dither")
    selectMode(savedMode.c_str());
  else drawCurrent();
  Serial.println("display ready; commands: wifi, ota, status, buttons, reboot, clock, date, life, snow, rain, dither, snake, pong, calibrate, font, effect, brightness, preview");
  connectWifi();
}

void loop() {
  while (Serial.available()) {
    char ch = char(Serial.read());
    if (ch == '\r') continue;
    if (ch == '\n') {
      command(serialLine);
      serialLine = "";
    } else if (serialLine.length() < 160) {
      serialLine += ch;
    } else {
      serialLine = "";
      Serial.println("command too long");
    }
  }
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wasConnected) {
    wasConnected = true;
    Serial.print("Wi-Fi connected, IP=");
    Serial.println(WiFi.localIP());
    if (!ntpRequested) {
      configTzTime("JST-9", "time.google.com", "pool.ntp.org");
      ntpRequested = true;
    }
    startOta();
    startWebStatus();
  } else if (!connected && wasConnected) {
    wasConnected = false;
    Serial.println("Wi-Fi disconnected");
  }
  if (!connected && !wifiSsid.isEmpty() && millis() - lastConnectAttempt > 30000)
    connectWifi();
  if (connected && !otaStarted) startOta();
  if (connected && !webStarted) startWebStatus();
  if (connected && otaStarted) ArduinoOTA.handle();
  if (connected) web.handleClient();
  if (connected && !mqttClient) startMqtt();
  if (connected && mqttConnected && mqttClient &&
      millis() - lastStatusPublish >= 30000) {
    if (esp_mqtt_client_publish(mqttClient, TOPIC_STATUS, "online", 0, 1, 1) >= 0)
      lastStatusPublish = millis();
    else
      ++mqttErrors;
  }
  checkTrialBoot();
  MqttCommand mqttCommand{};
  if (mqttCommands && xQueueReceive(mqttCommands, &mqttCommand, 0) == pdTRUE) {
    if (mqttCommand.kind == 1) startMqttText(mqttCommand.text, mqttCommand.length);
    else if (mqttCommand.kind == 3) {
      mqttEventAck(selectMode(mqttCommand.text) ? "mode" : "unsupported_mode");
    }
    else if (mqttCommand.kind == 4) {
      mqttEventAck(selectFont(mqttCommand.text) ? "font" : "unsupported_font");
    }
    else if (mqttCommand.kind == 5) {
      mqttEventAck(selectEffect(mqttCommand.text) ? "effect" : "unsupported_effect");
    }
    else if (mqttCommand.kind == 6) {
      previewClockTransition();
      mqttEventAck("preview");
    }
  }
  pollButtons();
  tickShow();
  if (stage == Stage::Idle && mode == DisplayMode::Clock) {
    int minute = int(time(nullptr) / 60);
    if (minute != lastClockMinute) {
      char target[6];
      formatClock(target);
      if (lastClockMinute >= 0 && clockEffect != ClockFaces::Effect::Off &&
          timeValid() && strcmp(shownClock, target) != 0)
        startClockTransition(target);
      else drawClock();
      lastClockMinute = minute;
    }
    tickClockTransition();
  }
  if (stage == Stage::Idle && mode == DisplayMode::Date) {
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    int day = local.tm_year * 366 + local.tm_yday;
    if (day != lastDateDay) {
      lastDateDay = day;
      drawCurrent();
    }
  }
  if (stage == Stage::Idle && (mode == DisplayMode::Seconds || mode == DisplayMode::Calendar)) {
    int second = int(time(nullptr));
    if (second != lastAuxSecond) { lastAuxSecond = second; drawCurrent(); }
  }
  if (stage == Stage::Idle && mode == DisplayMode::Life &&
      millis() - lastLife >= LIFE_MS) {
    lastLife = millis();
    bool changed = Patterns::stepLife();
    if (!changed || ++lifeGeneration % 30 == 0) Patterns::sparkLife();
    drawCurrent();
  }
  if (stage == Stage::Idle && mode == DisplayMode::Snow &&
      millis() - lastSnow >= SNOW_MS) {
    lastSnow = millis();
    Patterns::Snow::step();
    drawCurrent();
  }
  if (stage == Stage::Idle && mode == DisplayMode::Rain &&
      millis() - lastRain >= RAIN_MS) {
    lastRain = millis();
    Patterns::Rain::step();
    drawCurrent();
  }
  if (stage == Stage::Idle && mode == DisplayMode::Dither &&
      millis() - lastDither >= DITHER_MS) {
    lastDither = millis();
    Patterns::Dither::step();
    drawCurrent();
  }
  if (stage == Stage::Idle && mode == DisplayMode::Snake &&
      millis() - lastGame >= SNAKE_MS) {
    lastGame = millis();
    Games::Snake::step();
    drawCurrent();
  }
  if (stage == Stage::Idle && mode == DisplayMode::Pong &&
      millis() - lastGame >= PONG_MS) {
    lastGame = millis();
    Games::Pong::step();
    drawCurrent();
  }
  if (mode == DisplayMode::Pomodoro) Apps::Pomodoro::tick(millis());
  if (mode == DisplayMode::Stopwatch) Apps::Stopwatch::tick(millis());
  if (mode == DisplayMode::Countdown) Apps::Countdown::tick(millis());
  if (stage == Stage::Idle &&
      (mode == DisplayMode::Tempo || mode == DisplayMode::Pomodoro ||
       mode == DisplayMode::Stopwatch || mode == DisplayMode::Countdown ||
       mode == DisplayMode::Sky) &&
      millis() - lastAppFrame >= APP_FRAME_MS) {
    lastAppFrame = millis();
    if (mode == DisplayMode::Sky) Apps::Sky::step();
    drawCurrent();
  }
  delay(2);
}
