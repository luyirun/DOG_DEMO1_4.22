#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H

#include <esp-ai.h>
#include "mqtt_manager.h"


void setupESP_AI(ESP_AI& esp_ai);
void setupMQTT(MQTTManager& mqtt, void (*handleMessage)(const String &topic, const String &payload));
// void on_command(String command_id, String data);

#endif
    