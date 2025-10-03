#include "TelegramService.h"
#include <ArduinoJson.h>

/* ---------- ctor ---------- */
TelegramService::TelegramService(AsyncWebServer* srv,FS*fs,
                                 SecurityManager* sm,MultiWsManager* ws)
: StatefulService<TelegramSettings>(),
  _http(TelegramSettings::readForm, TelegramSettings::upd, this,
        srv, TEL_FORM_PATH, sm),
  _fs  (TelegramSettings::readConfig, TelegramSettings::upd, this, fs, TEL_FILE),
  _ws  (ws),
  _cli(),
  _bot("",_cli),
  _botTokenInBot("")
{
    if(_ws){
        _ws->addEndpoint<TelegramSettings>(TEL_WS_PATH, this,
                                           TelegramSettings::staRead,
                                           TelegramSettings::staUpd);
    }
    _q = xQueueCreate(TEL_Q_MAX, sizeof(TelegramQueuedMessage*));
}

/* ---------- begin ---------- */
void TelegramService::begin(){
    _fs.readFromFS();                 // читаємо конфіг з флеша (плоскі ключі)
    _cli.setInsecure();               // або setCACert(...) з кореневим сертом
    ensureBotToken(_state.botToken);  // ініціалізуємо fallback-бота

#if defined(CONFIG_FREERTOS_UNICORE) && CONFIG_FREERTOS_UNICORE
    xTaskCreate(task,"TelTask",6144,this,1,&_task);
#else
    xTaskCreatePinnedToCore(task,"TelTask",6144,this,1,&_task,1);
#endif

    // Пушимо заповнені поля у WS-UI
    callUpdateHandlers("sta");
}

/* ---------- legacy enqueue ---------- */
void TelegramService::addToQueue(const char* txt){
    if(!txt) return;
    enqueueMessage(String(), String(), String(txt));
}

/* ---------- нове enqueue ---------- */
void TelegramService::enqueueMessage(const String& chatId,
                                     const String& topicId,
                                     const String& text,
                                     const TelegramSendOptions& opt,
                                     const String& tokenOverride)
{
    if (text.isEmpty()) return;

    auto* m = new TelegramQueuedMessage();
    m->tokenOverride = tokenOverride;
    m->chatId        = chatId;
    m->topicId       = topicId;
    m->text          = text;
    m->parseMode     = opt.parseMode ? String(opt.parseMode) : String();
    m->silent        = opt.silent;

    if (xQueueSend(_q, &m, 0) != pdTRUE) {
        delete m;
        return;
    }
    _state.qSize = uxQueueMessagesWaiting(_q);
    callUpdateHandlers("sta"); // WS канал "sta"
}

/* ---------- TLS POST у Telegram API ---------- */
bool TelegramService::sendTelegramMessage(const String& token,
                                          const String& chatId,
                                          const String& topicId,
                                          const String& text,
                                          const char* parseMode,
                                          bool silent)
{
    if (token.isEmpty() || chatId.isEmpty()) return false;

    const char* host = "api.telegram.org";
    const int   port = 443;

    if(!_cli.connect(host, port)) return false;

    StaticJsonDocument<1024> body;
    body["chat_id"] = chatId;
    body["text"]    = text;
    if (parseMode && *parseMode) body["parse_mode"] = parseMode;
    if (!topicId.isEmpty())      body["message_thread_id"] = topicId.toInt();
    if (silent)                  body["disable_notification"] = true;

    String payload; serializeJson(body, payload);

    String url = String("/bot") + token + "/sendMessage";
    String req =
        String("POST ") + url + " HTTP/1.1\r\n" +
        "Host: " + host + "\r\n" +
        "User-Agent: ESP32\r\n" +
        "Content-Type: application/json\r\n" +
        "Connection: close\r\n" +
        "Content-Length: " + String(payload.length()) + "\r\n\r\n" +
        payload;

    _cli.print(req);

    // Очікуємо статус-лінію типу "HTTP/1.1 200 OK"
    String statusLine = _cli.readStringUntil('\n');
    bool ok = statusLine.indexOf("200") >= 0;

    // Дочитати/закрити
    while (_cli.connected() || _cli.available()) { (void)_cli.read(); }
    _cli.stop();
    return ok;
}

/* ---------- UTB fallback token ensure ---------- */
void TelegramService::ensureBotToken(const String& token){
    if (token == _botTokenInBot) return;
    _bot = UniversalTelegramBot(token.c_str(), _cli);
    _botTokenInBot = token;
}

/* ---------- допоміжні ---------- */
void TelegramService::appendLogLine(const String& line){
    if (_state.chatLogSize < TelegramSettings::LOG_MAX) {
        _state.chatLog[_state.chatLogSize++] = line;
    } else {
        for (uint8_t i=1;i<TelegramSettings::LOG_MAX;i++)
            _state.chatLog[i-1] = _state.chatLog[i];
        _state.chatLog[TelegramSettings::LOG_MAX-1] = line;
    }
    // оновимо склеєний read-only лог для форми
    String ro;
    for(uint8_t i=0;i<_state.chatLogSize;i++){
        if(i) ro += "\n";
        ro += _state.chatLog[i];
    }
    _state.manualLogRO = ro;

    callUpdateHandlers("sta");
}

void TelegramService::tryConsumeManualSendRequest(){
    if (_state.manualSendRequested && _state.enabled) {
        TelegramSendOptions opt;
        opt.parseMode = "Markdown";
        opt.silent    = false;

        // ставимо у чергу повідомлення з дефолтними chat/topic/token (з налаштувань сервісу)
        enqueueMessage(
            String(),             // chat override не заданий → візьмемо з сервісу
            String(),             // topic override не заданий → з сервісу
            _state.manualText,
            opt,
            String()              // token override не заданий → з сервісу
        );

        // echo у лог ще до фактичної відправки
        if (_state.manualText.length()){
            String line = String("[queued] ") + String(millis()/1000) + "s → " + _state.manualText;
            appendLogLine(line);
        }

        _state.manualSendRequested = false;
        _state.manualText = "";
        callUpdateHandlers("sta");
    }
}

/* ---------- worker task ---------- */
void TelegramService::task(void* pv){
    auto* s = static_cast<TelegramService*>(pv);
    TelegramQueuedMessage* m{nullptr};

    for(;;){
        // Підхопити "кнопку" Manual send (через WS/REST форму)
        s->tryConsumeManualSendRequest();

        if (xQueueReceive(s->_q, &m, pdMS_TO_TICKS(50))==pdPASS && m){
            const String effToken = !m->tokenOverride.isEmpty() ? m->tokenOverride : s->_state.botToken;
            const String effChat  = !m->chatId.isEmpty()        ? m->chatId        : s->_state.chatId;
            const String effTopic = !m->topicId.isEmpty()       ? m->topicId       : s->_state.topicId;
            const char*  effParse = (m->parseMode.length() ? m->parseMode.c_str() : nullptr);

            if(s->_state.enabled && !effChat.isEmpty() && !effToken.isEmpty()){
                // 1) Основний шлях — прямий POST
                bool ok = s->sendTelegramMessage(effToken, effChat, effTopic, m->text, effParse, m->silent);

                // 2) Фолбек — UniversalTelegramBot (без thread_id)
                if(!ok){
                    s->ensureBotToken(effToken);
                    s->_bot.sendMessage(effChat, m->text, effParse ? effParse : "Markdown");
                    ok = true; // якщо не впало — вважаємо ок
                }

                s->_state.lastMsg = m->text;
                s->_state.sent    = ok;

                // лог
                String line = String(ok ? "[ok] " : "[fail] ");
                line += String(millis()/1000) + "s → " + m->text;
                s->appendLogLine(line);
            }

            delete m;
            s->_state.qSize = uxQueueMessagesWaiting(s->_q);
            s->callUpdateHandlers("sta");
            vTaskDelay(pdMS_TO_TICKS(s->_state.sendDelay));
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}
