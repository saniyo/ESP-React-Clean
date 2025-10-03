#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <vector>
#include <map>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "StatefulService.h"

/* ---------- опис endpoint-у ---------- */
struct WsEndpointDesc{
    String              path;
    void*               stateService;          //  StatefulService<TState>*
    std::function<void(void*,JsonObject&)>     readFn;
    std::function<StateUpdateResult(JsonObject&,void*)> updateFn;
    AsyncWebSocket*     ws;
};

/* ---- елементи черг Tx / Rx ---- */
struct WsQueueItem   { String path; uint32_t cid; String payload; bool text; };
struct WsIncomingItem{ String path; uint32_t cid; String payload; bool text; };

/* ========================================================= */
class MultiWsManager{
public:
    explicit MultiWsManager(AsyncWebServer* server,size_t qSize=10)
      : _server(server),
        _pingIntervalSec(10),
        _pongTimeoutMs(30000),
        _lastPingTs(0),
        _hasFirstRtt(false),
        _alpha(0.3f),
        _avgRtt(0.f)
    {
        _txQ = xQueueCreate(qSize,sizeof(WsQueueItem*));
        _rxQ = xQueueCreate(qSize,sizeof(WsIncomingItem*));
        configASSERT(_txQ);  configASSERT(_rxQ);
    }
    ~MultiWsManager(){
        _pingTicker.detach();
        if(_txQ) vQueueDelete(_txQ);
        if(_rxQ) vQueueDelete(_rxQ);
        for(auto &d:_dsc) delete d.ws;
    }

    /* ---------- реєстрація endpoint-у ---------- */
    template<typename TState>
    void addEndpoint(const String& path,
                     StatefulService<TState>* svc,
                     std::function<void(TState&,JsonObject&)>      read,
                     std::function<StateUpdateResult(JsonObject&,TState&)> upd)
    {
        AsyncWebSocket* ws = new AsyncWebSocket(path.c_str());
        ws->onEvent([this,path](AsyncWebSocket* s,AsyncWebSocketClient* c,
                                AwsEventType t,void* a,uint8_t* d,size_t l)
        {
            for(auto &e:_dsc) if(e.path==path && e.ws==s){
                this->handleWsEvent(e,c,t,a,d,l);   // <–– прототип нижче
                return;
            }
        });
        _server->addHandler(ws);

        WsEndpointDesc e;
        e.path=path;  e.ws=ws;
        e.stateService=static_cast<void*>(svc);
        e.readFn=[read](void* p,JsonObject& root){
            read(*static_cast<TState*>(p),root);
        };
        e.updateFn=[upd](JsonObject& j,void* p){
            return upd(j,*static_cast<TState*>(p));
        };
        _dsc.push_back(e);
    }

    /* ==================== API ===================== */
    void enqueue(const String& path,uint32_t cid,const String& pl,bool txt){
        auto* it=new WsQueueItem{path,cid,pl,txt};
        if(xQueueSend(_txQ,&it,0)!=pdTRUE) delete it;
    }
    void broadcast(const String& path,const String& pl,bool txt=true){
        enqueue(path,0,pl,txt);
    }
    void sendTo(const String& path,uint32_t cid,const String& pl,bool txt=true){
        enqueue(path,cid,pl,txt);
    }

    /* --- push актуального стану всім клієнтам endpoint-а --- */
    void broadcastCurrentState(const String& path,const String& origin=""){
        StaticJsonDocument<2048> doc;
        JsonObject root=doc.to<JsonObject>();
        root["type"]="p"; root["origin_id"]=origin;
        JsonObject p=root.createNestedObject("p");

        for(auto &d:_dsc) if(d.path==path){ d.readFn(d.stateService,p); break; }

        String out; serializeJson(doc,out);
        broadcast(path,out,true);
    }

    void processAllQueues(){
        processRx(); processTx();
    }

    /* -------- Ping / Pong -------- */
    void beginPingPong(unsigned pingS=10){
        _pingIntervalSec=pingS;
        _pingTicker.attach((float)_pingIntervalSec,staticPingCb,this);
    }
    float averageRTT()const{ return _avgRtt; }

private:
    /* ---- internal ---- */
    AsyncWebServer*              _server;
    std::vector<WsEndpointDesc>  _dsc;
    QueueHandle_t                _txQ{},_rxQ{};
    Ticker                       _pingTicker;
    unsigned                     _pingIntervalSec;
    unsigned long                _pongTimeoutMs,_lastPingTs;
    std::map<uint32_t,unsigned long> _lastPong;
    bool                         _hasFirstRtt;
    float                        _alpha,_avgRtt;

    /* обробка Tx */
    void processTx(){
        WsQueueItem* it=nullptr;
        while(xQueueReceive(_txQ,&it,0)==pdTRUE){
            for(auto &d:_dsc) if(d.path==it->path){
                if(it->cid==0){
                    if(it->text) d.ws->textAll(it->payload);
                    else         d.ws->binaryAll((uint8_t*)it->payload.c_str(),it->payload.length());
                }else{
                    auto* c=d.ws->client(it->cid);
                    if(c && c->status()==WS_CONNECTED){
                        if(it->text) c->text(it->payload);
                        else         c->binary((uint8_t*)it->payload.c_str(),it->payload.length());
                    }
                }
                break;
            }
            delete it;
        }
    }

    /* обробка Rx */
    void processRx(){
        WsIncomingItem* it=nullptr;
        while(xQueueReceive(_rxQ,&it,0)==pdTRUE){
            StaticJsonDocument<1024> doc;
            if(!deserializeJson(doc,it->payload) && doc.is<JsonObject>()){
                JsonObject root=doc.as<JsonObject>();
                if(root.containsKey("p") && root["p"].is<JsonObject>()){
                    JsonObject p=root["p"];
                    for(auto &d:_dsc) if(d.path==it->path){
                        d.updateFn(p,d.stateService);
                        break;
                    }
                }
            }
            delete it;
        }
    }

    /* ---------- WS events ---------- */
    void handleWsEvent(WsEndpointDesc& d,AsyncWebSocketClient* c,
                       AwsEventType t,void* a,uint8_t* data,size_t len)
    {
        uint32_t id=c->id();
        switch(t){
        case WS_EVT_CONNECT:
            _lastPong[id]=millis();
            sendId(c);
            broadcastCurrentState(d.path,"ws_connect");
            break;
        case WS_EVT_PONG:
            _lastPong[id]=millis();
            if(_lastPingTs) updRtt(millis()-_lastPingTs);
            break;
        case WS_EVT_DISCONNECT: case WS_EVT_ERROR:
            _lastPong.erase(id);
            break;
        case WS_EVT_DATA:{
            auto* info=(AwsFrameInfo*)a;
            if(info->final && info->index==0 && info->opcode==WS_TEXT){
                String s((char*)data,len);
                enqueueRx(d.path,id,s,true);
            }}
            break;
        default: break;
        }
        processAllQueues();
    }

    void enqueueRx(const String& path,uint32_t cid,const String& pl,bool txt){
        auto* it=new WsIncomingItem{path,cid,pl,txt};
        if(xQueueSend(_rxQ,&it,0)!=pdTRUE) delete it;
    }

    void sendId(AsyncWebSocketClient* c){
        StaticJsonDocument<64> d; d["type"]="id"; d["id"]="ws:"+String(c->id());
        String s; serializeJson(d,s); c->text(s);
    }

    /* ---- RTT / ping ---- */
    static void staticPingCb(MultiWsManager* m){ if(m) { m->pingAll(); m->checkInactive(); } }
    void pingAll(){ _lastPingTs=millis(); for(auto &d:_dsc) d.ws->pingAll(); }
    void checkInactive(){
        unsigned long now=millis();
        for(auto it=_lastPong.begin();it!=_lastPong.end();){
            if(now-it->second>_pongTimeoutMs){ closeAll(it->first); it=_lastPong.erase(it);}
            else ++it;
        }
    }
    void updRtt(unsigned long rtt){ _avgRtt=_hasFirstRtt? _alpha*rtt+(1-_alpha)*_avgRtt : rtt; _hasFirstRtt=true; }
    void closeAll(uint32_t cid){ for(auto &d:_dsc){ if(auto* c=d.ws->client(cid)) c->close(); } }
};

/* ---- end of file ---- */
