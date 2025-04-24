#include "callbacks.h"
#include <Arduino.h>
#include "bool_init.h"

// 定义常量，避免硬编码
const String MQTT_TOPIC_APUB_BSUB = "Apub/Bsub";
const String MQTT_TOPIC_BPUB_ASUB = "Bpub/Asub";
const String ESPAI_EVENT_ON_IAT_CB = "on_iat_cb";
const String KEYWORD_FORWARD = "前进";
const String KEYWORD_BACKWARD = "后退";
const String KEYWORD_RIGHT = "右转";
const String KEYWORD_LEFT = "左转";
const String KEYWORD_SWING = "跳舞";
const String KEYWORD_STRETCH = "打招呼";
const String KEYWORD_SLEEP = "趴下";
const String KEYWORD_START = "起床啦";

// 统一处理ESPAI事件的回调函数
void esp_event_handler(String command_id, String data)
{
    // 打印接收到的事件信息
    Serial.print("[ESPAI事件] 类型: ");
    Serial.print(command_id);
    Serial.print(", 数据: ");
    Serial.println(data);

    // 处理语音识别结果，在异地模式下发送到MQTT
    if (command_id == ESPAI_EVENT_ON_IAT_CB && remoteMode)
    {
        // 对于A
        mqtt.publish("voice/controlB", data); // voice/controlA
        Serial.println("已发送语音到B端: " + data);
        // 对于B
        // mqtt.publish("voice/controlA", data);   //voice/controlA
        // Serial.println("已发送语音到A端: " + data);
        
    }
}

// 接收消息回调
void handleMessage(const String &topic, const String &payload)
{
    Serial.println("\n=== MQTT消息到达 ===");
    Serial.print("主题: ");
    Serial.println(topic);
    Serial.print("内容: ");
    Serial.println(payload);

    // 处理来自B端的消息（设备A写A，设备B写B）
    if (topic == "voice/controlA" && remoteMode) // voice/controlA
    {
        // 检查是否包含关键词
        bool foundKeyword = false;

        if (payload.indexOf(KEYWORD_FORWARD) >= 0)
        {
            move_forward();
            foundKeyword = true;
        }
        else if (payload.indexOf(KEYWORD_BACKWARD) >= 0)
        {
            move_backward();
            foundKeyword = true;
        }
        else if (payload.indexOf(KEYWORD_RIGHT) >= 0)
        {
            move_right();
            foundKeyword = true;
        }
        else if (payload.indexOf(KEYWORD_LEFT) >= 0)
        {
            move_left();
            foundKeyword = true;
        }
        else if (payload.indexOf(KEYWORD_SWING) >= 0)
        {
            move_swing();
            foundKeyword = true;
        }
        else if (payload.indexOf(KEYWORD_STRETCH) >= 0)
        {
            move_stretch();
            foundKeyword = true;
        }
        else if (payload.indexOf(KEYWORD_SLEEP) >= 0)
        {
            move_sleep();
            foundKeyword = true;
        }
        else if (payload == "start")
        {
            esp_ai.wakeUp();
        }
        // 如果没有发现关键词，播放语音内容
        else if (!foundKeyword && payload != "start" && payload !="sleep")
        {
            esp_ai.loop();
            vTaskDelay(pdMS_TO_TICKS(2500));
            esp_ai.tts(payload);
        }
        else if (payload == "sleep")
        {
           
            esp_ai.stopSession();
        }
    }

  
    else if (topic == "voice/controlA")//（设备A写A，设备B写B）
    {
        if (payload == "start")
        {
            esp_ai.wakeUp();
        }
    }
    
}

// // 处理MQTT消息的回调
// void mqtt_callback(const String &topic, const String &payload)
// {
//     Serial.print("[MQTT] Topic: ");
//     Serial.print(topic);
//     Serial.print(" | Payload: ");
//     Serial.println(payload);
// }


// void on_command(String command_id, String data){
//     Serial.println("on_command: " + command_id + ", " + data);
//     if (command_id == "on_iat_cb"){
//         Serial.println("on_iat_cb: " + data);
//         if (data == "退下吧"){
//             esp_ai.wakeUp();
//         }
//     }
// }


