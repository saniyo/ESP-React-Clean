#include "LightStateService.h"

LightStateService::LightStateService(AsyncWebServer*  server,
                                     SecurityManager* sm,
                                     AsyncMqttClient* mqtt,
                                     LightMqttSettingsService* lms,
                                     StatefulService<NTPSettings>* ntp,
                                     MultiWsManager*  ws)
: StatefulService<LightState>()
, _httpEndpoint(LightState::read,
                LightState::update,
                this, server,
                LIGHT_SETTINGS_ENDPOINT_PATH,
                sm, AuthenticationPredicates::IS_AUTHENTICATED)
, _mqttPubSub  (LightState::haRead, LightState::haUpdate, this, mqtt)
, _mqttClient  (mqtt)
, _lightMqttSettingsService(lms)
, _ntpService  (ntp)
, _wsManager   (ws)
{
    pinMode(LED_PIN, OUTPUT);
    _mqttClient->onConnect(std::bind(&LightStateService::registerConfig,this));
    _lightMqttSettingsService->addUpdateHandler([&](const String&){registerConfig();},false);
    _wsManager->addEndpoint<LightState>(LIGHT_SETTINGS_SOCKET_PATH,this,LightState::readSta,LightState::updateSta);
    addUpdateHandler([this](const String& origin){_wsManager->broadcastCurrentState(LIGHT_SETTINGS_SOCKET_PATH, origin);
    },false);
}

void LightStateService::begin() {
  _state.ledOn = DEFAULT_LED_STATE;

  // Запускаємо фоновий task
  xTaskCreatePinnedToCore(
    LightStateService::lightTask,
    "LightTask",
    4096,
    this,
    1,
    &lightTaskHandle,
    1
  );
}

void LightStateService::registerConfig() {
  if (!_mqttClient->connected()) {
    return;
  }
  String configTopic;
  String subTopic;
  String pubTopic;

  DynamicJsonDocument doc(256);
  _lightMqttSettingsService->read([&](LightMqttSettings& settings) {
    configTopic = settings.mqttPath + "/config";
    subTopic = settings.mqttPath + "/set";
    pubTopic = settings.mqttPath + "/state";
    doc["~"] = settings.mqttPath;
    doc["name"] = settings.name;
    doc["unique_id"] = settings.uniqueId;
  });
  doc["cmd_t"] = "~/set";
  doc["stat_t"] = "~/state";
  doc["schema"] = "json";
  doc["brightness"] = false;

  String payload;
  serializeJson(doc, payload);
  _mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
  _mqttPubSub.configureTopics(pubTopic, subTopic);
}

////////////////////////////////////////
//Task for light control
////////////////////////////////////////
void LightStateService::lightTask(void* pvParameters) {
  LightStateService* service = static_cast<LightStateService*>(pvParameters);
  while (true) {
    service->callUpdateHandlers("lightTask");  // Викликати оновлення для WS
    vTaskDelay(pdMS_TO_TICKS(1000));  // Приклад — 1s
  }
}