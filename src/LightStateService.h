#ifndef LIGHT_STATE_SERVICE_H
#define LIGHT_STATE_SERVICE_H

#include <Arduino.h>
#include <LightMqttSettingsService.h>
#include <HttpEndpoint.h>
#include <MqttPubSub.h>
// #include <WebSocketTxRx.h>
#include <NTPSettingsService.h>
#include <SunRise.h>
#include <FormBuilder.h>
#include <NewMultiWsService.h>  // <-- Містить MultiWsManager

#define LED_PIN 2

#define DEFAULT_LED_STATE false
#define OFF_STATE "OFF"
#define ON_STATE  "ON"

// Примітка: на більшості плат вбудований LED активний при LOW.
#ifdef ESP32
  #define LED_ON  0x1
  #define LED_OFF 0x0
#elif defined(ESP8266)
  #define LED_ON  0x0
  #define LED_OFF 0x1
#endif

#define LIGHT_SETTINGS_ENDPOINT_PATH "/rest/lightState"
#define LIGHT_SETTINGS_SOCKET_PATH   "/ws/lightState"

class LightState {
 public:
  bool   ledOn{DEFAULT_LED_STATE};
  float  monthlyConsumptionLimit{0.0f};  // kWh
  float  dailyConsumptionLimit{0.0f};    // kWh

  int    testNumber{0};
  int    testDropdown{2};
  int    gain{20};
  String testText;
  String textArea{"Millis are: "};

  // ---------- Генерація WS-стану (trend + тестові поля) ----------
  static void readSta(LightState& st, JsonObject& root) {
    /* -------------------------------------------------
     *  Конфігурація генератора
     * ------------------------------------------------*/
    constexpr uint8_t TREND_KEYS_TOTAL   = 21; // key1 … key21
    constexpr uint8_t TREND_KEYS_PER_MSG = 21; // скільки key-ів у WS-повідомленні
    constexpr bool    MERGE_KEYS_PER_TS  = true;

    // ---------- service flags ----------
    unsigned long now = millis();
    static bool toggle = false;
    static unsigned long lastT = 0;
    if (now - lastT >= 1000) { toggle = !toggle; lastT = now; }

    // ---------- math helpers ----------
    static double phase = 0.0;
    static double offs[TREND_KEYS_TOTAL];
    static bool   init = false;
    if (!init) {
      for (uint8_t i = 0; i < TREND_KEYS_TOTAL; i++) offs[i] = (rand() % 101) + 50;
      init = true;
    }

    static uint8_t keyIdx = 0;
    const  double  FREQ = 0.2, INC = 1.0, PHASE_RST = 1000.0;

    // ---------- контейнер для точок ----------
    JsonArray trendArr = root.createNestedArray("trend_data");

    // Порожній об'єкт для поточного timestamp (merge)
    JsonObject mergedPt;
    if (MERGE_KEYS_PER_TS) {
      mergedPt = trendArr.createNestedObject();
      mergedPt["timestamp"] = (unsigned long)(phase * 1000);
    }

    // ---------- цикл по ключах повідомлення ----------
    for (uint8_t sent = 0; sent < TREND_KEYS_PER_MSG; ++sent) {
      // кінець кола – скидаємо
      if (keyIdx >= TREND_KEYS_TOTAL) {
        keyIdx = 0;
        for (uint8_t i = 0; i < TREND_KEYS_TOTAL; i++) offs[i] = (rand() % 101) + 50;
        phase += INC;
        if (phase > PHASE_RST) phase = 0.0;
        if (MERGE_KEYS_PER_TS) {
          mergedPt = trendArr.createNestedObject();
          mergedPt["timestamp"] = (unsigned long)(phase * 1000);
        }
      }

      // ---- обчислення ----
      double A   = 50.0 + 10.0 * keyIdx + offs[keyIdx];
      double phi = keyIdx * 0.7;
      double s   = A * sin(FREQ * phase + phi);
      double c   = A * cos(FREQ * phase + phi);

      // ---- запис координати ----
      JsonObject pt = MERGE_KEYS_PER_TS ? mergedPt : trendArr.createNestedObject();
      if (!MERGE_KEYS_PER_TS) pt["timestamp"] = (unsigned long)(phase * 1000);

      switch (keyIdx) {
        case  0: pt["key1"]  = s;            break;
        case  1: pt["key2"]  = c;            break;
        case  2: pt["key3"]  = s + c;        break;
        case  3: pt["key4"]  = s * 2;        break;
        case  4: pt["key5"]  = c * 2;        break;
        case  5: pt["key6"]  = s - c;        break;
        case  6: pt["key7"]  = s * 1.5;      break;
        case  7: pt["key8"]  = c * 1.5;      break;
        case  8: pt["key9"]  = s + c * 0.5;  break;
        case  9: pt["key10"] = s * 0.5;      break;
        case 10: pt["key11"] = c * 0.5;      break;
        case 11: pt["key12"] = s + c * 1.2;  break;
        case 12: pt["key13"] = s * 0.8;      break;
        case 13: pt["key14"] = c * 0.8;      break;
        case 14: pt["key15"] = s + c * 1.8;  break;
        case 15: pt["key16"] = s * 2.5;      break;
        case 16: pt["key17"] = c * 2.5;      break;
        case 17: pt["key18"] = s - c * 0.3;  break;
        case 18: pt["key19"] = s * 3.0;      break;
        case 19: pt["key20"] = c * 3.0;      break;
        case 20: pt["key21"] = s + c * 2.2;  break;
      }
      keyIdx++;
    }

    // ---- тестові поля (ЛИШЕ boolean/number/string) ----
    root["test_text"]     = st.testText;
    root["test_number"]   = (int)(now + now); // просто якийсь рух
    root["test_checkbox"] = toggle;           // <-- boolean
    root["test_switch"]   = !toggle;          // <-- boolean
    root["test_textarea"] = st.textArea;
    root["test_dropdown"] = st.testDropdown;
  }

  // ---------- WS update (тільки boolean для булевих) ----------
  static StateUpdateResult updateSta(JsonObject& root, LightState& lightState) {
    bool stateChanged = false;
    Serial.println("Received WS object:");
    serializeJsonPretty(root, Serial);

    stateChanged |= FormBuilder::updateValue(root, "test_text",      lightState.testText);
    stateChanged |= FormBuilder::updateValue(root, "monthly_consumption_limit", lightState.monthlyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit",   lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on",         lightState.ledOn);       // bool only
    stateChanged |= FormBuilder::updateValue(root, "test_number",    lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown",  lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea",  lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "gain",           lightState.gain);

    return stateChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }

  // ---------- Видача форм (REST) ----------
  static void read(LightState& s, JsonObject& root) {
    JsonArray sta = FormBuilder::createForm(root, "status", "Status Form");
    FormBuilder::addTrendField(sta, "trend_data", AF::RW,
      lines(line("key1", hidden, "#8884d8", "monotone"),
            line("key2", hidden, "#FF0000", "monotone"),
            line("key3", hidden, "#FF00FF", "monotone")),
      xAxis("timestamp"), legend(true), tooltip(true), trendMaxPoints(120), mode("lineChart"));

    FormBuilder::addTrendField(sta, "trend_data", AF::RW,
      lines(line("key1", hidden, "#8884d8", "monotone"),
            line("key2", hidden, "#FF0000", "monotone"),
            line("key3", hidden, "#FF00FF", "monotone")),
      xAxis("timestamp"), legend(true), tooltip(true), trendMaxPoints(120), mode("barChart"));

    FormBuilder::addTrendField(sta, "trend_data", AF::RW,
      lines(line("key1",  visible, "#8884d8", "monotone"),
            line("key2",  hidden,  "#FF0000", "step"),
            line("key3",  visible, "#FF00FF", "monotone"),
            line("key21", visible, "#556B2F", "monotone")),
      xAxis("timestamp"), legend(true), tooltip(true), trendMaxPoints(120), mode("pieChart"));

    FormBuilder::addSwitchField  (sta, "led_on",       AF::RW, s.ledOn);
    FormBuilder::addSwitchField  (sta, "led_on",       AF::R,  s.ledOn);
    FormBuilder::addTextField    (sta, "test_text",    AF::RW, "Sample text from Medved");
    FormBuilder::addTextField    (sta, "test_text",    AF::R,  "Sample text from Medved");
    FormBuilder::addNumberField  (sta, "test_number",  AF::RW, (double)s.testNumber, minVal(0), maxVal(100), format("0.00"));
    FormBuilder::addNumberField  (sta, "test_number",  AF::R,  (double)s.testNumber, minVal(0), maxVal(100), format("0.00"));
    FormBuilder::addCheckboxField(sta, "test_checkbox",AF::RW, s.ledOn);
    FormBuilder::addCheckboxField(sta, "test_checkbox",AF::R,  s.ledOn);
    FormBuilder::addSwitchField  (sta, "test_switch",  AF::RW, s.ledOn);
    FormBuilder::addSwitchField  (sta, "test_switch",  AF::R,  s.ledOn);
    FormBuilder::addDropdownField(sta, "test_dropdown",AF::RW, s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option",3));
    FormBuilder::addDropdownField(sta, "test_dropdown",AF::R,  s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option",3));
    FormBuilder::addRadioField   (sta, "test_dropdown",AF::RW, s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option bla bla bla",3));
    FormBuilder::addRadioField   (sta, "test_dropdown",AF::R,  s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option bla bla bla",3));
    FormBuilder::addTextareaField(sta, "test_textarea",AF::RW, s.textArea);
    FormBuilder::addTextareaField(sta, "test_textarea",AF::R,  s.textArea);
    FormBuilder::addSliderField  (sta, "gain",        AF::RW, s.gain, minVal(10), maxVal(60));
    FormBuilder::addSliderField  (sta, "gain",        AF::R,  s.gain, minVal(10), maxVal(60));

    JsonArray set = FormBuilder::createForm(root, "settings", "Settings Form");
    FormBuilder::addTrendField(set, "trend_data", AF::RW,
      lines(line("key1", hidden, "#8884d8", "monotone"),
            line("key2", hidden, "#FF0000", "monotone"),
            line("key3", hidden, "#FF00FF", "monotone")),
      xAxis("timestamp"), legend(true), tooltip(true), trendMaxPoints(120), mode("lineChart"));

    FormBuilder::addTrendField(set, "trend_data", AF::RW,
      lines(line("key1", hidden, "#8884d8", "monotone"),
            line("key2", hidden, "#FF0000", "monotone"),
            line("key3", hidden, "#FF00FF", "monotone")),
      xAxis("timestamp"), legend(true), tooltip(true), trendMaxPoints(120), mode("barChart"));

    FormBuilder::addTrendField(set, "trend_data", AF::RW,
      lines(line("key1",  visible, "#8884d8", "monotone"),
            line("key2",  hidden,  "#FF0000", "step"),
            line("key3",  visible, "#FF00FF", "monotone"),
            line("key21", visible, "#556B2F", "monotone")),
      xAxis("timestamp"), legend(true), tooltip(true), trendMaxPoints(120), mode("pieChart"));

    FormBuilder::addSwitchField  (set, "led_on",       AF::RW, s.ledOn);
    FormBuilder::addSwitchField  (set, "led_on",       AF::R,  s.ledOn);
    FormBuilder::addTextField    (set, "test_text",    AF::RW, "Sample text from Medved");
    FormBuilder::addTextField    (set, "test_text",    AF::R,  "Sample text from Medved");
    FormBuilder::addNumberField  (set, "test_number",  AF::RW, (double)s.testNumber, minVal(0), maxVal(100), format("0.00"));
    FormBuilder::addNumberField  (set, "test_number",  AF::R,  (double)s.testNumber, minVal(0), maxVal(100), format("0.00"));
    FormBuilder::addCheckboxField(set, "test_checkbox",AF::RW, s.ledOn);
    FormBuilder::addCheckboxField(set, "test_checkbox",AF::R,  s.ledOn);
    FormBuilder::addSwitchField  (set, "test_switch",  AF::RW, s.ledOn);
    FormBuilder::addSwitchField  (set, "test_switch",  AF::R,  s.ledOn);
    FormBuilder::addDropdownField(set, "test_dropdown",AF::RW, s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option",3));
    FormBuilder::addDropdownField(set, "test_dropdown",AF::R,  s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option",3));
    FormBuilder::addRadioField   (set, "test_dropdown",AF::RW, s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option bla bla bla",3));
    FormBuilder::addRadioField   (set, "test_dropdown",AF::R,  s.testDropdown, opt("Option1",1), opt("Option2",2), opt("Third option bla bla bla",3));
    FormBuilder::addTextareaField(set, "test_textarea",AF::RW, s.textArea);
    FormBuilder::addTextareaField(set, "test_textarea",AF::R,  s.textArea);
    FormBuilder::addSliderField  (set, "gain",        AF::RW, s.gain, minVal(10), maxVal(60));
    FormBuilder::addSliderField  (set, "gain",        AF::R,  s.gain, minVal(10), maxVal(60));
  }

  // ---------- REST update (тільки boolean для булевих) ----------
  static StateUpdateResult update(JsonObject& root, LightState& lightState) {
    bool stateChanged = false;
    Serial.println("Received REST object :");
    serializeJsonPretty(root, Serial);

    stateChanged |= FormBuilder::updateValue(root, "test_text",      lightState.testText);
    stateChanged |= FormBuilder::updateValue(root, "monthly_consumption_limit", lightState.monthlyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit",   lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on",         lightState.ledOn);       // bool only
    stateChanged |= FormBuilder::updateValue(root, "test_number",    lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown",  lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea",  lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "gain",           lightState.gain);

    return stateChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }

  // ---------- Home Assistant сумісність (рядки ON/OFF як і було) ----------
  static void haRead(LightState& settings, JsonObject& root) {
    root["state"] = settings.ledOn ? ON_STATE : OFF_STATE;
  }

  static StateUpdateResult haUpdate(JsonObject& root, LightState& lightState) {
    String state = root["state"];
    bool newState = false;
    if (state.equals(ON_STATE))       newState = true;
    else if (state.equals(OFF_STATE)) newState = false;
    else return StateUpdateResult::ERROR;

    if (lightState.ledOn != newState) {
      lightState.ledOn = newState;
      return StateUpdateResult::CHANGED;
    }
    return StateUpdateResult::UNCHANGED;
  }
};

class LightStateService : public StatefulService<LightState> {
 public:
  /**
   * Конструктор приймає пок. на MultiWsManager, створений усередині ESP8266React.
   */
  LightStateService(AsyncWebServer* server,
                    SecurityManager* securityManager,
                    AsyncMqttClient* mqttClient,
                    LightMqttSettingsService* lightMqttSettingsService,
                    StatefulService<NTPSettings>* ntpService,
                    MultiWsManager* wsManager);

  void setOriginId(const String& id) { _originId = id; }
  String getOriginId() const { return _originId; }
  void begin();

 private:
  HttpEndpoint<LightState>           _httpEndpoint;
  MqttPubSub<LightState>             _mqttPubSub;
  AsyncMqttClient*                   _mqttClient;
  LightMqttSettingsService*          _lightMqttSettingsService;
  StatefulService<NTPSettings>*      _ntpService;
  TaskHandle_t                       lightTaskHandle;
  SunRise                            _sunRise;

  // Зберігаємо MultiWsManager
  MultiWsManager* _wsManager;
  String          _originId;

  void registerConfig();
  void controlLighting();
  static void lightTask(void* pvParameters);
};

#endif
