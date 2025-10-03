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
#define ON_STATE "ON"

// Note that the built-in LED is on when the pin is low on most NodeMCU boards.
// This is because the anode is tied to VCC and the cathode to the GPIO 4 (Arduino pin 2).
#ifdef ESP32
#define LED_ON 0x1
#define LED_OFF 0x0
#elif defined(ESP8266)
#define LED_ON 0x0
#define LED_OFF 0x1
#endif

#define LIGHT_SETTINGS_ENDPOINT_PATH "/rest/lightState"
#define LIGHT_SETTINGS_SOCKET_PATH "/ws/lightState"

class LightState {
 public:
  bool ledOn;
  float monthlyConsumptionLimit;  // Monthly consumption limit in kWh
  float dailyConsumptionLimit;    // Daily consumption limit in kWh

  int testNumber;
  int testDropdown = 2;
  int gain = 20;
  String testText;
  String textArea = "Millis are: ";

static void readSta(LightState& st, JsonObject& root)
{
    /* -------------------------------------------------
  *  Конфігурація генератора
  * ------------------------------------------------*/
  constexpr uint8_t  TREND_KEYS_TOTAL   = 21;   // key1 … key21
  constexpr uint8_t  TREND_KEYS_PER_MSG = 21;    // скільки key-ів у WS-повідомленні
  constexpr bool     MERGE_KEYS_PER_TS  = true; // true  ⇒  один timestamp
                                              // false ⇒  "timestamp + key" для кожного
  /* ---------- service flags ---------- */
  unsigned long now = millis();
  static bool toggle = false; static unsigned long lastT = 0;
  if(now-lastT >= 1000){ toggle=!toggle; lastT=now; }

  /* ---------- math helpers ---------- */
  static double phase = 0.0;
  static double offs [TREND_KEYS_TOTAL];
  static bool   init = false;
  if(!init){
    for(uint8_t i=0;i<TREND_KEYS_TOTAL;i++) offs[i]=(rand()%101)+50;
    init=true;
  }

  static uint8_t keyIdx = 0;
  const  double  FREQ   = 0.2, INC = 1.0, PHASE_RST = 1000.0;

  /* ---------- container for points ---------- */
  JsonArray trendArr = root.createNestedArray("trend_data");

  /* -- готуємо «порожній» об’єкт для даного timestamp,
   *    якщо обрано MERGE_KEYS_PER_TS == true
   * ------------------------------------------*/
  JsonObject mergedPt;
  if(MERGE_KEYS_PER_TS){
      mergedPt = trendArr.createNestedObject();
      mergedPt["timestamp"] = (unsigned long)(phase*1000);   // один раз
  }

  /* ---------- цикл по ключах у цьому повідомленні ---------- */
  for(uint8_t sent=0; sent<TREND_KEYS_PER_MSG; ++sent)
  {
    /* кінець кола – скидаємо */
    if(keyIdx >= TREND_KEYS_TOTAL){
      keyIdx=0;
      for(uint8_t i=0;i<TREND_KEYS_TOTAL;i++) offs[i]=(rand()%101)+50;
      phase += INC;  if(phase>PHASE_RST) phase=0.0;
      if(MERGE_KEYS_PER_TS){
        /* новий timestamp → новий об’єкт у масиві */
        mergedPt = trendArr.createNestedObject();
        mergedPt["timestamp"] = (unsigned long)(phase*1000);
      }
    }

    /* ---- обчислюємо значення ---- */
    double A   = 50.0 + 10.0*keyIdx + offs[keyIdx];
    double phi = keyIdx*0.7;
    double s   = A*sin(FREQ*phase + phi);
    double c   = A*cos(FREQ*phase + phi);

    /* ---- кладемо координату ---- */
    JsonObject pt = MERGE_KEYS_PER_TS ? mergedPt
                                      : trendArr.createNestedObject();

    if(!MERGE_KEYS_PER_TS) pt["timestamp"] = (unsigned long)(phase*1000);

    const char* keyName = nullptr;
    switch(keyIdx){
      case  0: keyName="key1";  pt[keyName]=s;           break;
      case  1: keyName="key2";  pt[keyName]=c;           break;
      case  2: keyName="key3";  pt[keyName]=s+c;         break;
      case  3: keyName="key4";  pt[keyName]=s*2;         break;
      case  4: keyName="key5";  pt[keyName]=c*2;         break;
      case  5: keyName="key6";  pt[keyName]=s-c;         break;
      case  6: keyName="key7";  pt[keyName]=s*1.5;       break;
      case  7: keyName="key8";  pt[keyName]=c*1.5;       break;
      case  8: keyName="key9";  pt[keyName]=s+c*0.5;     break;
      case  9: keyName="key10"; pt[keyName]=s*0.5;       break;
      case 10: keyName="key11"; pt[keyName]=c*0.5;       break;
      case 11: keyName="key12"; pt[keyName]=s+c*1.2;     break;
      case 12: keyName="key13"; pt[keyName]=s*0.8;       break;
      case 13: keyName="key14"; pt[keyName]=c*0.8;       break;
      case 14: keyName="key15"; pt[keyName]=s+c*1.8;     break;
      case 15: keyName="key16"; pt[keyName]=s*2.5;       break;
      case 16: keyName="key17"; pt[keyName]=c*2.5;       break;
      case 17: keyName="key18"; pt[keyName]=s-c*0.3;     break;
      case 18: keyName="key19"; pt[keyName]=s*3.0;       break;
      case 19: keyName="key20"; pt[keyName]=c*3.0;       break;
      case 20: keyName="key21"; pt[keyName]=s+c*2.2;     break;
    }
    keyIdx++;
  }

  /* ---- звичайні поля ---- */
  root["test_text"]     = st.testText;
  root["test_number"]   = now+now;
  root["test_checkbox"] = toggle? "1":"0";
  root["test_switch"]   = toggle? "0":"1";
  root["test_textarea"] = st.textArea;
  root["test_dropdown"] = st.testDropdown;
}

  static StateUpdateResult updateSta(JsonObject& root, LightState& lightState) {
    bool stateChanged = false;
    Serial.println("Received WS object:");
    serializeJsonPretty(root, Serial);
    stateChanged |= FormBuilder::updateValue(root, "test_text", lightState.testText);
    stateChanged |= FormBuilder::updateValue(root, "monthly_consumption_limit", lightState.monthlyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "gain", lightState.gain);
    return stateChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }

  static void read(LightState& s, JsonObject& root) {
    JsonArray sta = FormBuilder::createForm(root,"status","Status Form");
    FormBuilder::addTrendField(sta,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("lineChart"));
    FormBuilder::addTrendField(sta,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("barChart"));
    FormBuilder::addTrendField(sta,"trend_data",AF::RW,lines(line("key1",visible,"#8884d8","monotone"),line("key2",hidden,"#FF0000","step"),line("key3",visible,"#FF00FF","monotone"),line("key21",visible,"#556B2F","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("pieChart"));
    FormBuilder::addSwitchField(sta,"led_on",AF::RW,s.ledOn);          
    FormBuilder::addSwitchField(sta,"led_on",AF::R,s.ledOn);
    FormBuilder::addTextField (sta,"test_text",AF::RW,"Sample text from Medved"); 
    FormBuilder::addTextField(sta,"test_text",AF::R,"Sample text from Medved");
    FormBuilder::addNumberField(sta,"test_number",AF::RW,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00")); 
    FormBuilder::addNumberField(sta,"test_number",AF::R,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00"));
    FormBuilder::addCheckboxField(sta,"test_checkbox",AF::RW,s.ledOn); 
    FormBuilder::addCheckboxField(sta,"test_checkbox",AF::R,s.ledOn);
    FormBuilder::addSwitchField(sta,"test_switch",AF::RW,s.ledOn);     
    FormBuilder::addSwitchField(sta,"test_switch",AF::R,s.ledOn);
    FormBuilder::addDropdownField(sta,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3)); 
    FormBuilder::addDropdownField(sta,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3));
    FormBuilder::addRadioField(sta,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3)); 
    FormBuilder::addRadioField(sta,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3));
    FormBuilder::addTextareaField(sta,"test_textarea",AF::RW,s.textArea); 
    FormBuilder::addTextareaField(sta,"test_textarea",AF::R,s.textArea);
    FormBuilder::addSliderField(sta, "gain", AF::RW, s.gain, minVal(10), maxVal(60));
    FormBuilder::addSliderField(sta, "gain", AF::R, s.gain, minVal(10), maxVal(60));

    JsonArray set = FormBuilder::createForm(root,"settings","Settings Form");
    FormBuilder::addTrendField(set,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("lineChart"));
    FormBuilder::addTrendField(set,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("barChart"));
    FormBuilder::addTrendField(set,"trend_data",AF::RW,lines(line("key1",visible,"#8884d8","monotone"),line("key2",hidden,"#FF0000","step"),line("key3",visible,"#FF00FF","monotone"),line("key21",visible,"#556B2F","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("pieChart"));
    FormBuilder::addSwitchField(set,"led_on",AF::RW,s.ledOn);          
    FormBuilder::addSwitchField(set,"led_on",AF::R,s.ledOn);
    FormBuilder::addTextField (set,"test_text",AF::RW,"Sample text from Medved"); 
    FormBuilder::addTextField(set,"test_text",AF::R,"Sample text from Medved");
    FormBuilder::addNumberField(set,"test_number",AF::RW,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00")); 
    FormBuilder::addNumberField(set,"test_number",AF::R,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00"));
    FormBuilder::addCheckboxField(set,"test_checkbox",AF::RW,s.ledOn); 
    FormBuilder::addCheckboxField(set,"test_checkbox",AF::R,s.ledOn);
    FormBuilder::addSwitchField(set,"test_switch",AF::RW,s.ledOn);     
    FormBuilder::addSwitchField(set,"test_switch",AF::R,s.ledOn);
    FormBuilder::addDropdownField(set,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3)); 
    FormBuilder::addDropdownField(set,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3));
    FormBuilder::addRadioField(set,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3)); 
    FormBuilder::addRadioField(set,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3));
    FormBuilder::addTextareaField(set,"test_textarea",AF::RW,s.textArea); 
    FormBuilder::addTextareaField(set,"test_textarea",AF::R,s.textArea);
    FormBuilder::addSliderField(set, "gain", AF::RW, s.gain, minVal(10), maxVal(60));
    FormBuilder::addSliderField(set, "gain", AF::R, s.gain, minVal(10), maxVal(60));
}

  static StateUpdateResult update(JsonObject& root, LightState& lightState) {
    bool stateChanged = false;
    Serial.println("Received REST object :");
    serializeJsonPretty(root, Serial);
    stateChanged |= FormBuilder::updateValue(root, "test_text", lightState.testText);
    stateChanged |= FormBuilder::updateValue(root, "monthly_consumption_limit", lightState.monthlyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "gain", lightState.gain);

    return stateChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }

  static void haRead(LightState& settings, JsonObject& root) {
    root["state"] = settings.ledOn ? ON_STATE : OFF_STATE;
  }

  static StateUpdateResult haUpdate(JsonObject& root, LightState& lightState) {
    String state = root["state"];
    // parse new led state
    boolean newState = false;
    if (state.equals(ON_STATE)) {
      newState = true;
    } else if (!state.equals(OFF_STATE)) {
      return StateUpdateResult::ERROR;
    }
    // change the new state, if required
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

  void setOriginId(const String& id) {
    _originId = id;
  }
  String getOriginId() const {
    return _originId;
  }
  void begin();

 private:
  HttpEndpoint<LightState> _httpEndpoint;
  MqttPubSub<LightState> _mqttPubSub;
  AsyncMqttClient* _mqttClient;
  LightMqttSettingsService* _lightMqttSettingsService;
  StatefulService<NTPSettings>* _ntpService;
  TaskHandle_t lightTaskHandle;
  SunRise _sunRise;

  // Зберігаємо MultiWsManager
  MultiWsManager* _wsManager;
  String _originId;

  void registerConfig();
  void controlLighting();
  static void lightTask(void* pvParameters);
};

#endif
