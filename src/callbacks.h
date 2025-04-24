#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <String>
#include "esp-ai.h"
#include "mqtt_manager.h"

extern ESP_AI esp_ai;
extern MQTTManager mqtt;
extern const int led_pin;

// 定义常量，避免硬编码
extern const String ESPAI_EVENT_ON_IAT_CB;
extern const String MQTT_TOPIC_APUB_BSUB;
extern const String MQTT_TOPIC_BPUB_ASUB;
extern const String KEYWORD_FORWARD;
extern const String KEYWORD_BACKWARD;
extern const String KEYWORD_RIGHT;
extern const String KEYWORD_LEFT;
extern const String KEYWORD_SWING;
extern const String KEYWORD_STRETCH;
extern const String KEYWORD_SLEEP;

// 添加机器人动作函数的声明
extern void move_forward();
extern void move_backward();
extern void move_right();
extern void move_left();    
extern void move_swing();
extern void move_stretch();
extern void move_sleep();

// 添加外部互斥锁声明
extern SemaphoreHandle_t mqttMutex;
extern SemaphoreHandle_t servoMutex;
// 添加外部变量声明
// extern bool remoteMode;

void handleMessage(const String &topic, const String &payload);// 接收消息回调
// void on_command(String command_id, String data);
void speech_callback(String command_id, String data); // 语音回调
void setupMQTT(MQTTManager& mqtt, void (*handleMessage)(const String &topic, const String &payload));
#endif
    

    