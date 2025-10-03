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

/* ================= SETTINGS ================= */
struct TelegramSettings {
    /* runtime */
    String lastMsg;  int qSize{0}; bool sent{false};

    // Кільцевий лог для чату
    static constexpr uint8_t LOG_MAX = 50;
    String  chatLog[LOG_MAX];
    uint8_t chatLogSize{0};

    // Міні-чат
    String manualText;                  // текст для відправки
    String manualLogRO;                 // read-only лог (склеєний текст)
    bool   manualSendRequested{false};  // подія натискання кнопки

    // Конфіг
    String botToken, chatId, topicId;
    bool   enabled{false};
    unsigned long sendDelay{10000};

    /* ----- WS: sta (як у LightState) ----- */
    static void staRead(TelegramSettings& s, JsonObject& root){
        root["qsize"] = s.qSize;
        root["last"]  = s.lastMsg;
        root["sent"]  = s.sent;  s.sent = false;

        JsonArray log = root.createNestedArray("log");
        String joined;
        for(uint8_t i=0;i<s.chatLogSize;i++){
            log.add(s.chatLog[i]);
            if(i) joined += "\n";
            joined += s.chatLog[i];
        }
        root["m_text"] = s.manualText;
        root["m_log"]  = joined;

        root["m_send"] = false;
    }

    static StateUpdateResult staUpd(JsonObject& in, TelegramSettings& s){
        bool cfgChanged = false;

        // Runtime
        (void)FormBuilder::updateValue(in, "m_text", s.manualText);

        // Button: тригер по факту наявності ключа (значення не важливе)
        if (in.containsKey("m_send")) {
            s.manualSendRequested = true;
        }

        // Дозволимо редагувати конфіг через WS (опційно)
        cfgChanged |= FormBuilder::updateValue(in, "token", s.botToken);
        cfgChanged |= FormBuilder::updateValue(in, "chat",  s.chatId);
        cfgChanged |= FormBuilder::updateValue(in, "topic", s.topicId);

        bool enaTmp = s.enabled;
        if (FormBuilder::updateValue(in, "ena", enaTmp)) { s.enabled = enaTmp; cfgChanged = true; }

        if (in.containsKey("delay")) {
            unsigned long newDelay = s.sendDelay;
            if (in["delay"].is<uint32_t>() || in["delay"].is<int>() || in["delay"].is<unsigned long>()) {
                newDelay = (unsigned long)in["delay"].as<uint32_t>();
            } else if (in["delay"].is<const char*>()) {
                const char* cs = in["delay"].as<const char*>();
                if (cs && *cs) newDelay = strtoul(cs, nullptr, 10);
            }
            if (newDelay != s.sendDelay) { s.sendDelay = newDelay; cfgChanged = true; }
        }

        return cfgChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
    }

    /* ----- CONFIG (тільки ключі settings; для FS) ----- */
    static void readConfig(TelegramSettings& s, JsonObject& root) {
        root["token"] = s.botToken;
        root["chat"]  = s.chatId;
        root["topic"] = s.topicId;
        root["ena"]   = s.enabled;
        root["delay"] = s.sendDelay;
    }

    /* ----- REST форми ----- */
    static void readForm(TelegramSettings& s, JsonObject& root){
        // STATUS
        JsonArray st = FormBuilder::createForm(root,"status","Status");
        FormBuilder::addNumberField(st,"qsize",AF::R,s.qSize);
        FormBuilder::addTextField  (st,"last", AF::R,s.lastMsg.c_str());
        FormBuilder::addSwitchField(st,"sent", AF::R,s.sent);

        FormBuilder::addTextField  (st,"m_log",  AF::R,  s.manualLogRO.c_str());
        FormBuilder::addTextField  (st,"m_text", AF::RW, s.manualText.c_str());
        FormBuilder::addButtonField(st,"m_send", AF::RW, "Send");

        // SETTINGS
        JsonArray set=FormBuilder::createForm(root,"settings","Settings");
        FormBuilder::addTextField (set,"token", AF::RW,s.botToken.c_str());
        FormBuilder::addTextField (set,"chat",  AF::RW,s.chatId.c_str());
        FormBuilder::addTextField (set,"topic", AF::RW,s.topicId.c_str());
        FormBuilder::addSwitchField(set,"ena",  AF::RW,s.enabled);
        FormBuilder::addNumberField(set,"delay",AF::RW,s.sendDelay);
    }

    // Універсальний апдейт (REST + FS)
    static StateUpdateResult upd(JsonObject& j, TelegramSettings& s){
        bool cfgChanged = false;

        // Плоский формат або вкладений "settings"
        JsonObject src = j;
        if (j.containsKey("settings") && j["settings"].is<JsonObject>()) {
            src = j["settings"].as<JsonObject>();
        }

        cfgChanged |= FormBuilder::updateValue(src, "token", s.botToken);
        cfgChanged |= FormBuilder::updateValue(src, "chat",  s.chatId);
        cfgChanged |= FormBuilder::updateValue(src, "topic", s.topicId);

        bool enaTmp = s.enabled;
        if (FormBuilder::updateValue(src, "ena", enaTmp)) { s.enabled = enaTmp; cfgChanged = true; }

        if (src.containsKey("delay")) {
            unsigned long newDelay = s.sendDelay;
            if (src["delay"].is<uint32_t>() || src["delay"].is<int>() || src["delay"].is<unsigned long>()) {
                newDelay = (unsigned long)src["delay"].as<uint32_t>();
            } else if (src["delay"].is<const char*>()) {
                const char* cs = src["delay"].as<const char*>();
                if (cs && *cs) newDelay = strtoul(cs, nullptr, 10);
            }
            if (newDelay != s.sendDelay) { s.sendDelay = newDelay; cfgChanged = true; }
        }

        // Дамо REST право тригерити кнопку / текст (без запису у флеш)
        (void)FormBuilder::updateValue(src, "m_text", s.manualText);
        if (src.containsKey("m_send")) { s.manualSendRequested = true; }

        return cfgChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
    }
};

/* ================= Параметри відправки ================= */
struct TelegramSendOptions {
  const char* parseMode{"Markdown"};
  bool silent{false};
};

/* ================= Об'єкт черги ================= */
struct TelegramQueuedMessage {
  String tokenOverride;  // може бути порожнім → беремо token сервісу
  String chatId;         // може бути порожнім → беремо chat сервісу
  String topicId;        // може бути порожнім → беремо topic сервісу
  String text;
  String parseMode;      // "Markdown"/"HTML"/"" (без parse_mode)
  bool   silent{false};
};

/* ================= SERVICE ================= */
class TelegramService: public StatefulService<TelegramSettings>{
public:
    using CB = std::function<void(const char*)>;
    TelegramService(AsyncWebServer* srv,FS*fs,SecurityManager* sm,
                    MultiWsManager* ws);
    void begin();
    void loop(){}

    /* LEGACY */
    void addToQueue(const char* txt);

    /* Явна постановка повідомлення */
    void enqueueMessage(const String& chatId,
                        const String& topicId,
                        const String& text,
                        const TelegramSendOptions& opt = {},
                        const String& tokenOverride = String());
    void enqueueDefault(const char* txt){ if(txt) enqueueMessage(String(),String(),String(txt)); }

private:
    HttpEndpoint<TelegramSettings>  _http;
    FSPersistence<TelegramSettings> _fs;
    MultiWsManager* _ws;

    WiFiClientSecure                _cli;
    UniversalTelegramBot            _bot;              // fallback
    String                          _botTokenInBot;    // токен, з яким ініціалізовано _bot

    QueueHandle_t _q; TaskHandle_t _task{};

    static void task(void*);

    /* Основний шлях: прямий POST у Telegram (TLS) з підтримкою message_thread_id */
    bool sendTelegramMessage(const String& token,
                             const String& chatId,
                             const String& topicId,
                             const String& text,
                             const char* parseMode,
                             bool silent);

    /* Fallback: гарантувати правильний токен для UniversalTelegramBot */
    void ensureBotToken(const String& token);

    /* Допоміжне: лог та обробка Manual send */
    void appendLogLine(const String& line);
    void tryConsumeManualSendRequest();
};
