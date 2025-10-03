#include "ESP8266React.h"
// Якщо треба: #include "LightStateService.h"

ESP8266React::ESP8266React(AsyncWebServer* server)
  : _featureService(server),
    _securitySettingsService(server, &ESPFS),
    _wifiSettingsService(server, &ESPFS, &_securitySettingsService),
    _wifiScanner(server, &_securitySettingsService),
    _wifiStatus(server, &_securitySettingsService),
    _apSettingsService(server, &ESPFS, &_securitySettingsService),
    _apStatus(server, &_securitySettingsService, &_apSettingsService),
    _wsManager(server, 10),
#if FT_ENABLED(FT_NTP)
    _ntpSettingsService(server, &ESPFS, &_securitySettingsService),
    _ntpStatus(server, &_securitySettingsService),
#endif
#if FT_ENABLED(FT_OTA)
    _otaSettingsService(server, &ESPFS, &_securitySettingsService),
#endif
#if FT_ENABLED(FT_UPLOAD_FIRMWARE)
    _uploadFirmwareService(server, &_securitySettingsService),
#endif
#if FT_ENABLED(FT_MQTT)
    _mqttSettingsService(server, &ESPFS, &_securitySettingsService),
    _mqttStatus(server, &_mqttSettingsService, &_securitySettingsService),
#endif
#if FT_ENABLED(FT_SECURITY)
    _authenticationService(server, &_securitySettingsService),
#endif
#if FT_ENABLED(FT_TELEGRAM)
    _telegramService(server, &ESPFS, &_securitySettingsService, &_wsManager),
#endif
    _restartService(server, &_securitySettingsService),
    _factoryResetService(server, &ESPFS, &_securitySettingsService),
    _systemStatus(server, &_securitySettingsService)
{
  #ifdef PROGMEM_WWW
  WWWData::registerRoutes(
      [server, this](const String& uri, const String& contentType, const uint8_t* content, size_t len) {
        ArRequestHandlerFunction requestHandler = [contentType, content, len](AsyncWebServerRequest* request) {
          AsyncWebServerResponse* response = request->beginResponse(200, contentType, content, len);
          response->addHeader("Content-Encoding", "gzip");
          request->send(response);
        };
        server->on(uri.c_str(), HTTP_GET, requestHandler);
        if (uri.equals("/index.html")) {
          server->onNotFound([requestHandler](AsyncWebServerRequest* request) {
            if (request->method() == HTTP_GET) {
              requestHandler(request);
            } else if (request->method() == HTTP_OPTIONS) {
              request->send(200);
            } else {
              request->send(404);
            }
          });
        }
      });
#else
  server->serveStatic("/js/", ESPFS, "/www/js/");
  server->serveStatic("/css/", ESPFS, "/www/css/");
  server->serveStatic("/fonts/", ESPFS, "/www/fonts/");
  server->serveStatic("/app/", ESPFS, "/www/app/");
  server->serveStatic("/favicon.ico", ESPFS, "/www/favicon.ico");
  server->onNotFound([](AsyncWebServerRequest* request) {
    if (request->method() == HTTP_GET) {
      request->send(ESPFS, "/www/index.html");
    } else if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404);
    }
  });
#endif

#if defined(ENABLE_CORS)
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", CORS_ORIGIN);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Accept, Content-Type, Authorization");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Credentials", "true");
#endif
}

void ESP8266React::begin() {
#ifdef ESP32
  ESPFS.begin(true);
#elif defined(ESP8266)
  ESPFS.begin();
#endif
  _wifiSettingsService.begin();
  _apSettingsService.begin();
#if FT_ENABLED(FT_NTP)
  _ntpSettingsService.begin();
#endif
#if FT_ENABLED(FT_OTA)
  _otaSettingsService.begin();
#endif
#if FT_ENABLED(FT_MQTT)
  // _mqttSettingsService.begin();
#endif
#if FT_ENABLED(FT_SECURITY)
  _securitySettingsService.begin();
#endif
#if FT_ENABLED(FT_TELEGRAM)
  _telegramService.begin();
#endif
}

void ESP8266React::loop() {
  _wifiSettingsService.loop();
  _apSettingsService.loop();
  // _wsManager.processRxQueueInLoop();
  _wsManager.processAllQueues();
#if FT_ENABLED(FT_OTA)
  _otaSettingsService.loop();
#endif
#if FT_ENABLED(FT_MQTT)
  // _mqttSettingsService.loop();
#endif
#if FT_ENABLED(FT_TELEGRAM)
  // ...
#endif
}
