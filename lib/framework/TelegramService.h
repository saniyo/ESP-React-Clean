#pragma once
#include <StatefulService.h>
#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <FormBuilder.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <NewMultiWsService.h>

#define TEL_FORM_PATH   "/rest/telegramForm"
#define TEL_WS_PATH     "/ws/telegramStatus"
#define TEL_FILE        "/config/telegram.json"
#define TEL_Q_MAX       20

struct TelegramSettings {
    /* runtime */
    String lastMsg;  int qSize{0}; bool sent{false};

    /* config  */
    String botToken, chatId, topicId;
    bool   enabled{false};
    unsigned long sendDelay{10000};

    /* ----- status ----- */
    static void staRead(TelegramSettings&s, JsonObject& r){
        JsonObject f=r.createNestedObject("status")
                      .createNestedObject("fields");
        f["qsize"]=s.qSize;     f["last"]=s.lastMsg;
        f["sent"]=s.sent;       s.sent=false;
    }
    static StateUpdateResult staUpd(JsonObject&,TelegramSettings&){
        return StateUpdateResult::UNCHANGED;
    }

    /* ----- forms ----- */
    static void readForm(TelegramSettings&s, JsonObject& root){
        JsonArray st = FormBuilder::createForm(root,"status","Queue");
        FormBuilder::addNumberField(st,"qsize",AF::R,s.qSize);
        FormBuilder::addTextField  (st,"last", AF::R,s.lastMsg.c_str());
        FormBuilder::addSwitchField(st,"sent", AF::R,s.sent);

        JsonArray set=FormBuilder::createForm(root,"settings","Telegram cfg");
        FormBuilder::addTextField (set,"token", AF::RW,s.botToken.c_str());
        FormBuilder::addTextField (set,"chat",  AF::RW,s.chatId.c_str());
        FormBuilder::addTextField (set,"topic", AF::RW,s.topicId.c_str());
        FormBuilder::addSwitchField(set,"ena",  AF::RW,s.enabled);
        FormBuilder::addNumberField(set,"delay",AF::RW,s.sendDelay);
    }
    static StateUpdateResult upd(JsonObject& j,TelegramSettings&s){
        s.botToken  = j["token"]  | s.botToken;
        s.chatId    = j["chat"]   | s.chatId;
        s.topicId   = j["topic"]  | s.topicId;
        s.enabled   = j["ena"]    | s.enabled;
        s.sendDelay = j["delay"]  | s.sendDelay;
        return StateUpdateResult::CHANGED;
    }
};

/* ================= SERVICE ================= */
class TelegramService: public StatefulService<TelegramSettings>{
public:
    using CB = std::function<void(const char*)>;
    TelegramService(AsyncWebServer* srv,FS*fs,SecurityManager* sm,
                    MultiWsManager* ws);
    void begin();   void loop(){}

    /* external producers -> enqueue */
    void addToQueue(const char* txt);

private:
    HttpEndpoint<TelegramSettings>  _http;
    FSPersistence<TelegramSettings> _fs;
    MultiWsManager* _ws;
    UniversalTelegramBot           _bot;
    WiFiClientSecure               _cli;

    QueueHandle_t _q; TaskHandle_t _task{};
    static void task(void*);
};
