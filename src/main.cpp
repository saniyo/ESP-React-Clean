#include <ESP8266React.h>
#include <LightMqttSettingsService.h>
#include <LightStateService.h>
#include <SoftwareSerial.h>

#define SERIAL_BAUD_RATE 115200

AsyncWebServer server(80);

ESP8266React esp8266React(&server);
LightMqttSettingsService lightMqttSettingsService =
    LightMqttSettingsService(&server, esp8266React.getFS(), esp8266React.getSecurityManager());
LightStateService lightStateService = LightStateService(&server,
                                                        esp8266React.getSecurityManager(),
                                                        esp8266React.getMqttClient(),
                                                        &lightMqttSettingsService,
                                                        esp8266React.getNTPSettingsService(),
                                                        esp8266React.getWsManager());

void espTask1(void* pvParameters);
TaskHandle_t espTaskHandle1 = NULL;

void espTask2(void* pvParameters);
TaskHandle_t espTaskHandle2 = NULL;

void setup() {
  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);

  // start the framework and demo project
  esp8266React.begin();

  // load the initial light settings
  lightStateService.begin();

  // start the light service
  // lightMqttSettingsService.begin();
  // lightStateService.gpsSetup();

  // start the server
  server.begin();

  // // create task for ESPAsyncWebServer event loop
  xTaskCreatePinnedToCore(espTask1, "ESPAsyncWebServerTask", 4096, NULL, 1, &espTaskHandle1, 0);
  // // create task for ESPAsyncWebServer event loop
  xTaskCreatePinnedToCore(espTask2, "ESPAsyncWebServerTask", 2048, NULL, 1, &espTaskHandle2, 0);
  
}

void loop() {

}

void espTask1(void* pvParameters) {
  // loop forever
  while (true) {
    esp8266React.loop();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void espTask2(void* pvParameters) {
  // loop forever
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}