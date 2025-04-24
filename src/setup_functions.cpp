#include "setup_functions.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "callbacks.h"

// void on_command(String command_id, String data)
// {
//     if (remoteMode)
//     {
//         if ((command_id == "on_llm_cb" || command_id == "on_iat_cb"))

//         { 
//            esp_ai.stopSession();
//         }
//     }
// }

void setupESP_AI(ESP_AI &esp_ai)
{
    esp_ai.setVolume(1.0);

    // ESP-AI 配置
    bool debug = true;
    ESP_AI_wifi_config wifi_config = {"", "", "ESP-AI"};
    ESP_AI_server_config server_config = {};
    ESP_AI_wake_up_config wake_up_config = {"serial", 1, 10, "start"};
    // strcpy(wake_up_config.wake_up_scheme, "asrpro"); // 天问唤醒
    strcpy(wake_up_config.str, "start");

    // 同步时间
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    // strcpy(wake_up_config.wake_up_scheme, "pin_high");//按钮唤醒
    // wake_up_config.pin = 10;

    esp_ai.begin({debug, wifi_config, server_config, wake_up_config});

    // esp_ai.onEvent(on_command);
}

void setupMQTT(MQTTManager &mqtt, void (*handleMessage)(const String &topic, const String &payload))
{
    // 启动MQTT
    if (!mqtt.begin())
    {
        Serial.println("致命错误: MQTT初始化失败!");
    }
    // mqtt.subscribe("Apub/Bsub");
    // mqtt.subscribe("Bpub/Asub");
    // 添加异地模式下的主题订阅
    mqtt.subscribe("voice/controlA"); // A选这个
    // mqtt.subscribe("voice/controlB");    //B选这个
    mqtt.setCallback(handleMessage);
}