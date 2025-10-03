#include "PZEMService.h"
#if FT_ENABLED(FT_PZEM)

/* ---------- ctor ---------- */
PZEMService::PZEMService(AsyncWebServer* srv, FS* fs, SecurityManager* sm,
                         HardwareSerial* hs, CB cb, MultiWsManager* ws)
  : StatefulService<PZEMSettings>(),
    _http (PZEMSettings::readForm, PZEMSettings::updateSettings,
           this, srv, PZEM_SETTINGS_SERVICE_PATH, sm),
    _fs   (PZEMSettings::readForm, PZEMSettings::updateSettings,
           this, fs, PZEM_SETTINGS_FILE),
    _ws   (ws),
    _hs   (hs),
    _dev  (*hs, _state.RX, _state.TX, _state.ADR),
    _cb   (cb)
{
    _ws->addEndpoint<PZEMSettings>(PZEM_SETTINGS_SOCKET_PATH, this, PZEMSettings::staRead, nullptr);
    addUpdateHandler([this](const String& origin){_ws->broadcastCurrentState(PZEM_SETTINGS_SOCKET_PATH, origin);},false);
}

/* ---------- life-cycle ---------- */
void PZEMService::begin(){
    _fs.readFromFS();
    _hs->begin(9600, SERIAL_8N1, _state.RX, _state.TX);
    _dev.setAddress(_state.ADR);

    xTaskCreatePinnedToCore(task,"PZEM",4096,this,1,&_tsk,1);
}

void PZEMService::loop(){ /* nothing – усе у task */ }

/* ---------- RTOS task ---------- */
void PZEMService::task(void* pv){
    auto* s = static_cast<PZEMService*>(pv);
    for(;;){
        s->poll();
        vTaskDelay(pdMS_TO_TICKS(s->_state.updMs));
    }
}

/* ---------- poll + notify ---------- */
void PZEMService::poll(){
    _dev.updateValues();

    _state.U  = _dev.voltage();
    _state.I  = _dev.current();
    _state.P  = _dev.power();
    _state.E  = _dev.energy();
    _state.F  = _dev.frequency();
    _state.PF = _dev.pf();
    _state.ok = !isnan(_state.U) && !isnan(_state.F);

    if(_state.rst){ _dev.resetEnergy(); _state.rst=false; }

    callUpdateHandlers("pzemTask");

    if(_state.tg && _cb){
        String m="PZEM\nU:"+String(_state.U)+
                 "\nI:"+String(_state.I)+
                 "\nP:"+String(_state.P)+
                 "\nE:"+String(_state.E);
        notifyTG(m);
    }
}

void PZEMService::notifyTG(const String& m){ if(_cb) _cb(m.c_str()); }

#endif /* FT_ENABLED(FT_PZEM) */
