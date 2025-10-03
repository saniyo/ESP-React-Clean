#include "GPSService.h"
#include <Arduino.h>

#if FT_ENABLED(FT_GPS)

/* ------------ ctor ------------ */
GPSService::GPSService(AsyncWebServer* srv, FS* fs, SecurityManager* sm,
                       SoftwareSerial* ss, MessageCallback cb,
                       MultiWsManager*  ws)
  : StatefulService<GPSSettings>(),
    _httpEndpoint (GPSSettings::readForm, GPSSettings::updateSettings,
                   this, srv, GPS_SETTINGS_SERVICE_PATH, sm),
    _fsPersistence(GPSSettings::readForm, GPSSettings::updateSettings,
                   this, fs, GPS_SETTINGS_FILE),
    _ws(ws), _ss(ss), _cb(cb)
{
  _ws->addEndpoint<GPSSettings>(GPS_SETTINGS_SOCKET_PATH,
                                this,
                                GPSSettings::staRead,
                                GPSSettings::staUpdate);
  _wsRegistered = true;
}
/* ------------ begin ------------ */
void GPSService::begin(){
  _fsPersistence.readFromFS();
  gpsInit();

  _ss->begin(9600, SWSERIAL_8N1, _state.RX_PIN, -1);

  xTaskCreatePinnedToCore(gpsTask,"GPSTask",8192,this,1,&_taskHandle,1);
  Serial.println(F("GPSService started"));
}

/* ------------ loop ------------ */
void GPSService::loop(){
  gpsRead();
  gpsCheckTimeout();
}

/* ------------ helpers ------------ */
void GPSService::gpsInit(){ _ss->begin(9600,SWSERIAL_8N1,_state.RX_PIN,-1); }

void GPSService::gpsRead(){ while(_ss->available()) _gps.encode(_ss->read()); }

bool GPSService::isAccurate(){
  if(_state.satellites < _state.MIN_SATELLITES)          return false;
  if(_state.hdop      > _state.MAX_HDOP)                 return false;
  return (millis()-_state.lastUpdateTime) <= _state.MAX_UPDATE_INTERVAL;
}

void GPSService::handleData(){
  _state.latitude    = _gps.location.lat();
  _state.longitude   = _gps.location.lng();
  _state.altitude    = _gps.altitude.meters();
  _state.speed       = _gps.speed.kmph();
  _state.course      = _gps.course.deg();
  _state.satellites  = _gps.satellites.value();
  _state.hdop        = _gps.hdop.value();

  if(_gps.date.isValid() && _gps.time.isValid()){
    char d[11],t[9];
    sprintf(d,"%02d/%02d/%02d",_gps.date.month(),_gps.date.day(),_gps.date.year());
    sprintf(t,"%02d:%02d:%02d",_gps.time.hour(),_gps.time.minute(),_gps.time.second());
    _state.date=d; _state.time=t; _state.lastUpdateTime=millis();
  }
  _state.valid = isAccurate();

  /* тригеримо WS-слухачів */
  callUpdateHandlers("ws");

  if(_state.notifyTelegram && _cb){
    String msg = "Service: GPS\nLat: "+String(_state.latitude)+
                 "\nLon: "+String(_state.longitude)+
                 "\nAlt: "+String(_state.altitude)+
                 "\nSpeed: "+String(_state.speed)+
                 "\nCourse: "+String(_state.course)+
                 "\nSat: "+String(_state.satellites)+
                 "\nHDOP: "+String(_state.hdop);
    notifyTG(msg);
  }
}

void GPSService::gpsCheckTimeout(){
  if(millis()-_state.lastUpdateTime > 5000){
    _state.latitude=_state.longitude=_state.altitude=_state.speed=_state.course=_state.hdop=0;
    _state.satellites=0; _state.valid=false;
  }
}

void GPSService::propagateData(){ callUpdateHandlers("gps"); }

void GPSService::notifyTG(const String& m){ if(_cb) _cb(m.c_str()); }

/* ------------ task ------------ */
void GPSService::gpsTask(void* pv){
  GPSService* s = static_cast<GPSService*>(pv);
  for(;;){
    if(s->_gps.location.isUpdated()) s->handleData();
    vTaskDelay(pdMS_TO_TICKS(s->_state.MESSAGE_INTERVAL));
  }
}

#endif   /* FT_ENABLED(FT_GPS) */
