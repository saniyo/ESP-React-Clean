#include "TelegramService.h"
#include <ArduinoJson.h>

/* ---------- ctor ---------- */
TelegramService::TelegramService(AsyncWebServer* srv,FS*fs,
                                 SecurityManager* sm,MultiWsManager* ws)
: StatefulService<TelegramSettings>(),
  _http(TelegramSettings::readForm,TelegramSettings::upd,this,
        srv,TEL_FORM_PATH,sm),
  _fs  (TelegramSettings::readForm,TelegramSettings::upd,this,fs,TEL_FILE),
  _ws  (ws), _cli(), _bot("",_cli)
{
    if(_ws){
        _ws->addEndpoint<TelegramSettings>(TEL_WS_PATH,this,
                                           TelegramSettings::staRead,
                                           TelegramSettings::staUpd);
    }
    _q = xQueueCreate(TEL_Q_MAX,sizeof(char*));
}

/* ---------- begin ---------- */
void TelegramService::begin(){
    _fs.readFromFS();
    _cli.setInsecure();
    _bot = UniversalTelegramBot(_state.botToken.c_str(),_cli);

    xTaskCreatePinnedToCore(task,"TelTask",6144,this,1,&_task,1);
}

/* ---------- enqueue ---------- */
void TelegramService::addToQueue(const char* txt){
    if(!txt) return;
    char* c = strdup(txt);
    if(xQueueSend(_q,&c,0)!=pdTRUE) free(c);
    _state.qSize = uxQueueMessagesWaiting(_q);
    callUpdateHandlers("ws");
}

/* ---------- task ---------- */
void TelegramService::task(void* pv){
    auto* s = static_cast<TelegramService*>(pv);
    char* msg;
    for(;;){
        if(xQueueReceive(s->_q,&msg,portMAX_DELAY)==pdPASS && msg){
            if(s->_state.enabled){
                s->_bot.sendMessage(s->_state.chatId,msg,"Markdown");
                s->_state.lastMsg = msg; s->_state.sent = true;
            }
            free(msg);
            s->_state.qSize = uxQueueMessagesWaiting(s->_q);
            s->callUpdateHandlers("ws");
            vTaskDelay(pdMS_TO_TICKS(s->_state.sendDelay));
        }
    }
}
