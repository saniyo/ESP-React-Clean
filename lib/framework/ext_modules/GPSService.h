#ifndef GPSService_h
#define GPSService_h

#include <SettingValue.h>
#include <StatefulService.h>
#include <FSPersistence.h>
#include <HttpEndpoint.h>
#include <FormBuilder.h>          //  <-- NEW
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <NewMultiWsService.h>    //  <-- NEW
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define GPS_SETTINGS_FILE        "/config/gpsSettings.json"
#define GPS_SETTINGS_SERVICE_PATH "/rest/gpsForm"
#define GPS_SETTINGS_SOCKET_PATH  "/ws/gpsStatus"

#define STX 1
#define SRX 0

#if FT_ENABLED(FT_GPS)

/* ====================== модель стану ====================== */
class GPSSettings {
 public:
  /* dynamic data ------------- */
  float latitude{0}, longitude{0}, altitude{0}, speed{0}, course{0}, hdop{0};
  int   satellites{0};
  bool  valid{false};
  String date, time;
  unsigned long lastUpdateTime{0};

  /* config params ------------ */
  int   RX_PIN{0}, MIN_SATELLITES{4};
  float MAX_HDOP{1000};
  unsigned long MAX_UPDATE_INTERVAL{50000}, MAX_TIME_DIFF{600000}, MESSAGE_INTERVAL{5000};
  bool  notifyTelegram{false};

  /* ---------- WS: STATUS ---------- */
 static void staRead(GPSSettings& s, JsonObject& root){
    /* root → { status:{ fields:{ …live data… } } } */
    JsonObject f = root.createNestedObject("status")
                        .createNestedObject("fields");

    f["lat"]  = s.latitude;    f["lng"] = s.longitude;
    f["alt"]  = s.altitude;    f["spd"] = s.speed;
    f["crs"]  = s.course;      f["sat"] = s.satellites;
    f["hdop"] = s.hdop;        f["valid"] = s.valid;
    f["date"] = s.date;        f["time"] = s.time;
  }

  static StateUpdateResult staUpdate(JsonObject&, GPSSettings&){
    return StateUpdateResult::UNCHANGED;          //  runtime-форму не редагуємо
  }

  /* -------------   2. UI-каркас (readSettings)  -------------- */
  static void readForm(GPSSettings& s, JsonObject& root){

    /* ---------- status-форма (RO) ---------- */
    JsonArray st = FormBuilder::createForm(root,"status","GPS status");

    FormBuilder::addTextField  (st,"date",  AF::R, s.date.c_str());
    FormBuilder::addTextField  (st,"time",  AF::R, s.time.c_str());
    FormBuilder::addNumberField(st,"lat",   AF::R, s.latitude , format("0.000000"));
    FormBuilder::addNumberField(st,"lng",   AF::R, s.longitude, format("0.000000"));
    FormBuilder::addNumberField(st,"alt",   AF::R, s.altitude , format("0.0"));
    FormBuilder::addNumberField(st,"spd",   AF::R, s.speed    , format("0.0"));
    FormBuilder::addNumberField(st,"crs",   AF::R, s.course   , format("0.0"));
    FormBuilder::addNumberField(st,"sat",   AF::R, s.satellites);
    FormBuilder::addNumberField(st,"hdop",  AF::R, s.hdop     , format("0.0"));
    FormBuilder::addSwitchField(st,"valid", AF::R, s.valid);

    /* ---------- settings-форма (RW) ---------- */
    JsonArray set = FormBuilder::createForm(root,"settings","GPS settings");

    FormBuilder::addNumberField(set,"rx_pin",           AF::RW, s.RX_PIN);
    FormBuilder::addNumberField(set,"min_satellites",   AF::RW, s.MIN_SATELLITES,  minVal(1), maxVal(12));
    FormBuilder::addNumberField(set,"max_hdop",         AF::RW, s.MAX_HDOP,        minVal(0), maxVal(5000));
    FormBuilder::addNumberField(set,"max_update_ms",    AF::RW, s.MAX_UPDATE_INTERVAL);
    FormBuilder::addNumberField(set,"max_time_diff_ms", AF::RW, s.MAX_TIME_DIFF);
    FormBuilder::addNumberField(set,"msg_interval_ms",  AF::RW, s.MESSAGE_INTERVAL);
    FormBuilder::addSwitchField (set,"notify_tg",       AF::RW, s.notifyTelegram);
  }

  /* --------------   3. apply-збереження  -------------------- */
  static StateUpdateResult updateSettings(JsonObject& r, GPSSettings& s){
    s.RX_PIN              = r["rx_pin"]              | s.RX_PIN;
    s.MIN_SATELLITES      = r["min_satellites"]      | s.MIN_SATELLITES;
    s.MAX_HDOP            = r["max_hdop"]            | s.MAX_HDOP;
    s.MAX_UPDATE_INTERVAL = r["max_update_ms"]       | s.MAX_UPDATE_INTERVAL;
    s.MAX_TIME_DIFF       = r["max_time_diff_ms"]    | s.MAX_TIME_DIFF;
    s.MESSAGE_INTERVAL    = r["msg_interval_ms"]     | s.MESSAGE_INTERVAL;
    s.notifyTelegram      = r["notify_tg"]           | s.notifyTelegram;
    return StateUpdateResult::CHANGED;
  }
};

/* ====================== SERVICE ====================== */
class GPSService : public StatefulService<GPSSettings> {
 public:
  using MessageCallback = std::function<void(const char*)>;

  GPSService(AsyncWebServer* srv,
           FS*             fs,
           SecurityManager*sm,
           SoftwareSerial* ss,
           MessageCallback cb,
           MultiWsManager* ws); 

  void begin();          void loop();
  void propagateData();  // manual broadcast
  static void gpsTask(void*);

 private:
  /* REST & FS */
  HttpEndpoint<GPSSettings>   _httpEndpoint;
  FSPersistence<GPSSettings>  _fsPersistence;

  /* WS */
  MultiWsManager* _ws;
  bool _wsRegistered{false};

  /* GPS */
  TinyGPSPlus     _gps;
  SoftwareSerial* _ss;
  TaskHandle_t    _taskHandle{nullptr};

  /* misc */
  MessageCallback _cb;

  /* helpers */
  void gpsInit();      void gpsRead();      void gpsCheckTimeout();
  bool isAccurate();   void handleData();   void notifyTG(const String& msg);
};

#endif   /* FT_ENABLED(FT_GPS) */
#endif   /* GPSService_h */
