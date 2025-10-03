#pragma once
/* ------------- PZEM-004T service (Web, WS, Telegram) -------------- */
#include <SettingValue.h>
#include <StatefulService.h>
#include <FSPersistence.h>
#include <HttpEndpoint.h>
#include <FormBuilder.h>
#include <NewMultiWsService.h>

#include <PZEM004Tv30.h>
#include <HardwareSerial.h>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define PZEM_SETTINGS_FILE         "/config/pzemSettings.json"
#define PZEM_SETTINGS_SERVICE_PATH "/rest/pzemForm"
#define PZEM_SETTINGS_SOCKET_PATH  "/ws/pzemStatus"

#if FT_ENABLED(FT_PZEM)

/* ===================== модель стану ===================== */
struct PZEMSettings {
  /* live data */ float U{0}, I{0}, P{0}, E{0}, F{0}, PF{0}; bool ok{false}, rst{false};

  /* cfg */ int RX{7}, TX{6}, ADR{0xF8};
  float Uthr{220}, Ithr{100}, Pthr{2200}, Ethr{2200};
  unsigned long updMs{5000};
  bool tg{false};          // повідомляти в Telegram?

  /* ---------------- WS: status ---------------- */
  static void staRead(PZEMSettings& s, JsonObject& r){
      // JsonObject t = r.createNestedObject("status").createNestedObject("fields").createNestedObject("trend_data");
      // JsonObject statusObj = r.createNestedObject("status");
      // JsonObject fieldsObj = statusObj.createNestedObject("fields");
      JsonArray trendArray = r.createNestedArray("trend_data");
      JsonObject point = trendArray.createNestedObject();
      point["timestamp"] = (unsigned long)(millis());  // «псевдо-час»
      point["u"] = s.U;  point["i"] = s.I;  point["p"] = s.P;  point["e"] = s.E;
      point["f"] = s.F;  point["pf"] = s.PF; point["ok"] = s.ok;

      r["u"] = s.U;  r["i"] = s.I;  r["p"] = s.P;  r["e"] = s.E;
      r["f"] = s.F;  r["pf"] = s.PF; r["ok"] = s.ok;
  }
  static StateUpdateResult staUpdate(JsonObject&, PZEMSettings&){ return StateUpdateResult::UNCHANGED; }

  /* ---------------- REST/GUI: read форму ---------------- */
  static void readForm(PZEMSettings& s, JsonObject& root){
      /* status (RO) */
      JsonArray sta = FormBuilder::createForm(root,"status","PZEM status");
      FormBuilder::addTrendField(sta,"trend_data",AF::R,lines(line("u",hidden,"#8884d8","monotone"),line("f",hidden,"#FF0000","monotone"),line("pf",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("lineChart"));
      FormBuilder::addNumberField(sta,"u",  AF::R,s.U,  format("0.0"));
      FormBuilder::addNumberField(sta,"i",  AF::R,s.I,  format("0.00"));
      FormBuilder::addNumberField(sta,"p",  AF::R,s.P,  format("0"));
      FormBuilder::addNumberField(sta,"e",  AF::R,s.E,  format("0.000"));
      FormBuilder::addNumberField(sta,"f",  AF::R,s.F,  format("0.0"));
      FormBuilder::addNumberField(sta,"pf", AF::R,s.PF, format("0.00"));
      FormBuilder::addSwitchField (sta,"ok",AF::R,s.ok);

      /* settings (RW) */
      JsonArray set = FormBuilder::createForm(root,"settings","PZEM settings");
      FormBuilder::addNumberField(set,"rx",      AF::RW,s.RX);
      FormBuilder::addNumberField(set,"tx",      AF::RW,s.TX);
      FormBuilder::addNumberField(set,"adr",     AF::RW,s.ADR);
      FormBuilder::addNumberField(set,"u_thr",   AF::RW,s.Uthr);
      FormBuilder::addNumberField(set,"i_thr",   AF::RW,s.Ithr);
      FormBuilder::addNumberField(set,"p_thr",   AF::RW,s.Pthr);
      FormBuilder::addNumberField(set,"e_thr",   AF::RW,s.Ethr);
      FormBuilder::addNumberField(set,"upd_ms",  AF::RW,s.updMs);
      FormBuilder::addSwitchField(set,"notify_tg",AF::RW,s.tg);
  }

  /* ---------------- REST/GUI: apply -------------------- */
  static StateUpdateResult updateSettings(JsonObject& j, PZEMSettings& s){
      s.RX     = j["rx"]      | s.RX;
      s.TX     = j["tx"]      | s.TX;
      s.ADR    = j["adr"]     | s.ADR;
      s.Uthr   = j["u_thr"]   | s.Uthr;
      s.Ithr   = j["i_thr"]   | s.Ithr;
      s.Pthr   = j["p_thr"]   | s.Pthr;
      s.Ethr   = j["e_thr"]   | s.Ethr;
      s.updMs  = j["upd_ms"]  | s.updMs;
      s.tg     = j["notify_tg"] | s.tg;
      return StateUpdateResult::CHANGED;
  }
};

/* ===================== SERVICE ===================== */
class PZEMService : public StatefulService<PZEMSettings>{
public:
  using CB = std::function<void(const char*)>;

  PZEMService(AsyncWebServer* srv, FS* fs, SecurityManager* sm,
              HardwareSerial* hs, CB cb, MultiWsManager* ws);

  void begin();   void loop();

private:
  /* REST / FS */
  HttpEndpoint<PZEMSettings>  _http;
  FSPersistence<PZEMSettings> _fs;

  /* WS */
  MultiWsManager* _ws{nullptr};

  /* HW */
  HardwareSerial* _hs;
  PZEM004Tv30     _dev;
  TaskHandle_t    _tsk{nullptr};

  /* misc */
  CB _cb;
  void notifyTG(const String& msg);

  /* task */
  static void task(void* pv);
  void poll();
};

#endif /* FT_ENABLED(FT_PZEM) */
