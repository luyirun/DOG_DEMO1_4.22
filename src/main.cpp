#include <esp-ai.h>
#include "mqtt_manager.h"
#include "setup_functions.h"
#include "callbacks.h"
#include <ESP32Servo.h>
#include <Arduino.h>
#include <OneButton.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Face.h"
#include "freertos/semphr.h"
#include "bool_init.h"

ESP_AI esp_ai;

#define SERVO1_PIN 39 // 左后
#define SERVO2_PIN 36 // 右前
#define SERVO3_PIN 38 // 右后
#define SERVO4_PIN 37 // 左前
#define ASR_TX_PIN 12
#define ASR_RX_PIN 11
#define TOUCH_PIN 3
#define body_sensor 8

Servo servos[4];
int move_delay = 150;
int move_speed = 4;

String currentAction = "IDLE";

HardwareSerial asrSerial(1);
OneButton button(TOUCH_PIN, false);

// 面部对象
Face *face;

// 定义动作与表情的映射
const String actionEmotions[] = {
    "",
    "Focused",
    "Worried",
    "Curious",
    "Curious",
    "Happy",
    "Relaxed"};

unsigned long previousMillis = 0;
const long interval = 3000;     // 定时器间隔为 5 秒
unsigned long timerCounter = 0; // 新增：定时器计数器

// 在main.cpp顶部添加状态机变量
enum MoveState
{
    MOVE_IDLE,
    MOVE_STEP1,
    MOVE_STEP2,
    MOVE_STEP3
};
MoveState currentMoveState = MOVE_IDLE;
unsigned long moveTimer = 0;

// 异地模式相关变量
// bool remoteMode = false;
unsigned long longPressStartTime = 0;
const long remoteModeThreshold = 3000; // 长按5秒进入异地模式

// 添加互斥锁用于保护共享资源
SemaphoreHandle_t mqttMutex;
SemaphoreHandle_t servoMutex;

// 函数前向声明
void recordActionState(String action);
void setEmotion(String emotion);
void reset_servos();
void click();
void doubleClick();
void longPressStart();
void longPressStop();
uint16_t angle(uint8_t deg);
void move_servos(int servo1_angle, int servo2_angle, int servo3_angle, int servo4_angle, int delay_time);
void move_forward();
void move_backward();
void move_right();
void move_left();
void move_swing();
void move_stretch();
void move_sleep();
void reset_to_stand();
void esp_event_handler(String command_id, String data);

// 线程函数声明
void buttonCheckTask(void *pvParameters);
void emotionUpdateTask(void *pvParameters);
void esp_aiTask(void *pvParameters);
void servo_task(void *pvParameters);
void MQTT_task(void *pvParameters);
// void stackMonitorTask(void *pvParameters); // 添加堆栈监测任务声明
void bodySearchTask(void *pvParameters);
// void monitor_task(void *pvParams);
// 任务句柄，用于堆栈监测
TaskHandle_t buttonTaskHandle = NULL;  // 1
TaskHandle_t emotionTaskHandle = NULL; // 2
TaskHandle_t espAiTaskHandle = NULL;   // 3
TaskHandle_t servoTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t bodySearchTaskHandle = NULL;

// MQTT配置（注意检查用户名密码）
MQTTManager mqtt(
    "a5df2d0f.ala.cn-hangzhou.emqxsl.cn", // MQTT服务器
    8883,                                 // 端口
    "sub-client-A",                       // 客户端ID（需唯一）
    "emqx",                               // 检查服务器实际用户名
    "public",                             // 检查服务器实际密码
    MQTTManager::getCACert());

void setup()
{
    Serial.begin(115200);

    asrSerial.begin(115200, SERIAL_8N1, ASR_TX_PIN, ASR_RX_PIN); // ASRPRO 串口初始化
    // 舵机初始化
    for (int i = 0; i < 4; i++)
    {
        servos[i].setPeriodHertz(50);
        servos[i].attach((i == 0) ? SERVO1_PIN : (i == 1) ? SERVO2_PIN
                                             : (i == 2)   ? SERVO3_PIN
                                                          : SERVO4_PIN,
                         500, 2500);
    }
    setupESP_AI(esp_ai);
    pinMode(body_sensor, INPUT);
    setupMQTT(mqtt, handleMessage); // mqtt消息处理函数.语音合成
                                    // 注册回调函数
    button.attachClick(click);
    button.attachDoubleClick(doubleClick);
    button.attachLongPressStart(longPressStart);
    button.attachLongPressStop(longPressStop);

    reset_servos(); // 初始化时让舵机立正

    // 创建一个新的面部对象，设置屏幕宽度、高度和眼睛大小
    face = new Face(/* screenWidth = */ 128, /* screenHeight = */ 64, /* eyeSize = */ 40);
    // 设置当前表情为正常
    face->Expression.GoTo_Normal();
    // 自动在不同的行为之间切换（基于分配给每种情绪的权重随机选择新的行为）
    face->RandomBehavior = true;

    // 自动眨眼
    face->RandomBlink = true;
    // 设置眨眼的间隔时间
    face->Blink.Timer.SetIntervalMillis(2000);

    // 自动选择新的随机方向看
    face->RandomLook = true;

    esp_ai.onEvent(esp_event_handler);
    // 修改任务创建部分的堆栈大小参数
    xTaskCreatePinnedToCore(
        buttonCheckTask,   // 线程函数
        "buttonCheckTask", // 线程名称
        3000,
        NULL,
        2,
        NULL, // 添加句柄用于堆栈监控
        ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
        emotionUpdateTask,
        "EmotionUpdateTask",
        2048,
        NULL,
        1,
        &emotionTaskHandle,
        !ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
        esp_aiTask,
        "EspAiTask",
        1024 * 8,
        NULL,
        4,
        &espAiTaskHandle,
        ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
        servo_task,
        "ServoTask",
        1024 * 4,
        NULL,
        3,
        &servoTaskHandle,
        !ARDUINO_RUNNING_CORE);

    // 创建堆栈监测任务
    // xTaskCreatePinnedToCore(
    //     stackMonitorTask,      // 线程函数
    //     "StackMonitor",        // 线程名称
    //     2248,                  // 堆栈大小
    //     NULL,                  // 传递给线程函数的参数
    //     1,                     // 优先级
    //     NULL,                  // 任务句柄
    //     ARDUINO_RUNNING_CORE); // 固定到的核心

    xTaskCreatePinnedToCore(
        MQTT_task,              // 线程函数
        "MQTT_task",            // 线程名称
        1024 * 5,               // 堆栈大小
        NULL,                   // 传递给线程函数的参数
        2,                      // 优先级
        &mqttTaskHandle,        // 任务句柄
        !ARDUINO_RUNNING_CORE); // 固定到的核心

    xTaskCreatePinnedToCore(
        bodySearchTask,         // 线程函数
        "bodySearchTask",       // 线程名称
        1024 * 3,               // 堆栈大小
        NULL,                   // 传递给线程函数的参数
        1,                      // 优先级
        &bodySearchTaskHandle,  // 任务句柄
        !ARDUINO_RUNNING_CORE); // 固定到的核心

    // 创建互斥锁
    mqttMutex = xSemaphoreCreateMutex();
    servoMutex = xSemaphoreCreateMutex();
}

void loop()
{
}

void setEmotion(String emotion)
{
    if (emotion == "Normal")
    {
        face->Expression.GoTo_Normal();
    }
    else if (emotion == "Angry")
    {
        face->Expression.GoTo_Angry();
    }
    else if (emotion == "Happy")
    {
        face->Expression.GoTo_Happy();
    }
    else if (emotion == "Sad")
    {
        face->Expression.GoTo_Sad();
    }
    else if (emotion == "Worried")
    {
        face->Expression.GoTo_Worried();
    }
    else if (emotion == "Focused")
    {
        face->Expression.GoTo_Focused();
    }
    else if (emotion == "Annoyed")
    {
        face->Expression.GoTo_Annoyed();
    }
    else if (emotion == "Surprised")
    {
        face->Expression.GoTo_Surprised();
    }
    else if (emotion == "Skeptic")
    {
        face->Expression.GoTo_Skeptic();
    }
    else if (emotion == "Frustrated")
    {
        face->Expression.GoTo_Frustrated();
    }
    else if (emotion == "Unimpressed")
    {
        face->Expression.GoTo_Unimpressed();
    }
    else if (emotion == "Sleepy")
    {
        face->Expression.GoTo_Sleepy();
    }
    else if (emotion == "Suspicious")
    {
        face->Expression.GoTo_Suspicious();
    }
    else if (emotion == "Squint")
    {
        face->Expression.GoTo_Squint();
    }
    else if (emotion == "Furious")
    {
        face->Expression.GoTo_Furious();
    }
    else if (emotion == "Scared")
    {
        face->Expression.GoTo_Scared();
    }
    else if (emotion == "Awe")
    {
        face->Expression.GoTo_Awe();
    }
    else
    {
        Serial.println("未知的表情：" + emotion);
    }
}

// 角度转换函数：将0~180转换为舵机对应的脉宽（单位微秒）
uint16_t angle(uint8_t deg)
{
    return map(deg, 0, 180, 500, 2500);
}

// 舵机立正
void reset_servos()
{
    for (int i = 0; i < 4; i++)
    {
        servos[i].writeMicroseconds(angle(90));
    }
    delay(100);
}

// 使用互斥锁保护MQTT发布
bool safeMQTTPublish(const String &topic, const String &payload)
{
    if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        bool result = mqtt.publish(topic, payload);
        xSemaphoreGive(mqttMutex);
        return result;
    }
    return false;
}

// 修改舵机动作函数，加入互斥锁保护
void move_servos(int servo1_angle, int servo2_angle, int servo3_angle, int servo4_angle, int delay_time)
{
    if (xSemaphoreTake(servoMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        servos[0].writeMicroseconds(angle(servo1_angle));
        servos[1].writeMicroseconds(angle(servo2_angle));
        servos[2].writeMicroseconds(angle(servo3_angle));
        servos[3].writeMicroseconds(angle(servo4_angle));
        delay(delay_time);
        xSemaphoreGive(servoMutex);
    }
}

// 前进
void move_forward()
{
    recordActionState("MOVING_FORWARD"); // 记录动作状态
    setEmotion("Focused");
    for (int i = 0; i < 5; i++)
    {
        move_servos(135, 90, 90, 45, move_delay);
        move_servos(90, 45, 135, 90, move_delay);
        move_servos(90, 90, 90, 90, move_delay);
        move_servos(90, 135, 45, 90, move_delay);
        move_servos(45, 90, 90, 135, move_delay);
        move_servos(90, 90, 90, 90, move_delay);
    }
}

// 后退
void move_backward()
{
    recordActionState("MOVING_BACKWARD");
    setEmotion("Surprised");
    for (int i = 0; i < 5; i++)
    {
        move_servos(45, 90, 90, 135, move_delay);
        move_servos(90, 135, 45, 90, move_delay);
        move_servos(90, 90, 90, 90, move_delay);
        move_servos(90, 45, 135, 90, move_delay);
        move_servos(135, 90, 90, 45, move_delay);
        move_servos(90, 90, 90, 90, move_delay);
    }
}

// 右转
void move_right()
{
    recordActionState("MOVING_RIGHT");
    setEmotion("Surprised");
    for (int i = 0; i < 5; i++)
    {
        move_servos(45, 90, 90, 45, move_delay);
        move_servos(90, 135, 135, 90, move_delay);
        move_servos(90, 90, 90, 90, move_delay);
    }
}

// 左转
void move_left()
{
    recordActionState("MOVING_LEFT");
    setEmotion("Surprised");
    for (int i = 0; i < 5; i++)
    {
        move_servos(90, 135, 135, 90, move_delay);
        move_servos(45, 90, 90, 45, move_delay);
        move_servos(90, 90, 90, 90, move_delay);
    }
}

// 摇摆
void move_swing()
{
    recordActionState("MOVING_SWING");
    setEmotion("happy");
    for (int i = 0; i < 4; i++)
    {
        move_servos(130, 130, 50, 50, 200);
        move_servos(50, 50, 130, 130, 200);
    }
}

// 坐下招手（用于双击触摸和语音指令0x06）
void move_stretch()
{
    recordActionState("MOVING_STRETCH");
    setEmotion("Happy");
    reset_servos();
    for (uint8_t i = 0; i < 70; i++)
    {
        move_servos(90 + i, 90, 90 - i, 90, move_speed);
    }
    delay(1000);
    for (uint8_t i = 0; i < 70; i++)
    {
        move_servos(160 - i, 90, 20 + i, 90, move_speed);
    }
    for (uint8_t i = 0; i < 65; i++)
    {
        move_servos(90, 90 + i, 90, 90 - i, move_speed);
    }

    delay(1000);
    move_servos(140, 145, 90, 15, 1000);
    move_servos(178, 145, 90, 15, 500);
    move_servos(140, 145, 90, 15, 500);
    move_servos(178, 145, 90, 15, 500);
    move_servos(140, 145, 90, 15, 500);
    move_servos(70, 145, 90, 15, 0);
}

// 趴下
void move_sleep()
{
    recordActionState("MOVING_SLEEP");
    setEmotion("Sleepy");
    for (uint8_t i = 0; i < 75; i++)
    {
        move_servos(90 - i, 90, 90 + i, 90, move_speed);
    }
    for (uint8_t i = 0; i < 75; i++)
    {
        move_servos(15, 90 + i, 165, 90 - i, move_speed);
    }
}

// 恢复立正状态
void reset_to_stand()
{
    recordActionState("RESET_TO_STAND");
    setEmotion("Normal");
    for (int i = 0; i < 4; i++)
    {
        servos[i].writeMicroseconds(angle(90));
    }
}

void move_stand()
{
    recordActionState("MOVE_STAND");
    setEmotion("Normal");
    reset_servos();
    delay(1000); // 保持站立状态1秒
}

// 单击回调函数
void click()
{
    move_swing();
    Serial.println("单击");
    reset_to_stand();
}

// 双击回调函数
void doubleClick()
{
    move_stretch();
    Serial.println("双击");
    reset_to_stand();
}

// 长按回调函数
void longPressStart()
{
    longPressStartTime = millis();
    Serial.println("开始长按");
}

// 长按结束回调函数
void longPressStop()
{
    unsigned long pressDuration = millis() - longPressStartTime;
    if (pressDuration >= remoteModeThreshold)
    {
        remoteMode = !remoteMode;
        if (remoteMode)
        {
            setEmotion("Happy");
            esp_ai.tts("已进入异地模式");
            mqtt.subscribe("voice/controlA"); // 订阅从B发来的消息
            Serial.println("已进入异地模式");
        }
        else
        {
            setEmotion("Normal");
            esp_ai.tts("已退出异地模式");
            Serial.println("已退出异地模式");
        }
    }
    Serial.println("结束长按");
}

// 触摸按钮检测线程函数
void buttonCheckTask(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        button.tick(); // 轮询按钮状态
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    vTaskDelete(NULL); // 删除任务
}

void emotionUpdateTask(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        face->Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void esp_aiTask(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        if (remoteMode)
        {
            esp_ai_start_ed = "0";
            esp_ai_session_id = "";
        }
        else
        {
            esp_ai.loop();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    vTaskDelete(NULL);
}

void servo_task(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        if (asrSerial.available())
        {
            char commandChar = asrSerial.read();
            if (!isdigit(commandChar))
            {
                continue;
            }
            int command = commandChar - '0';
            if (command >= 1 && command <= 7)
            {
                setEmotion(actionEmotions[command]);
                switch (command)
                {
                case 1:
                    move_forward();
                    break;
                case 2:
                    move_backward();
                    break;
                case 3:
                    move_right();
                    break;
                case 4:
                    move_left();
                    break;
                case 5:
                    move_swing();
                    break;
                case 6:
                    move_stretch();
                    break;
                case 7:
                    move_sleep();
                    break;
                }
                Serial.println("Received command: " + String(command));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void MQTT_task(void *pvParams)
{
    while (1)
    {
        mqtt.loop();    // 处理MQTT消息
        vTaskDelay(10); // 避免忙等待
    }
    vTaskDelete(NULL); // 删除任务
}

void recordActionState(String action)
{
    currentAction = action;
    Serial.println("当前动作状态: " + currentAction);
}

void bodySearchTask(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        int sensorValue = digitalRead(body_sensor);
        if (sensorValue == HIGH && currentAction == "MOVING_SLEEP")
        {
            reset_servos();
            Serial.println("有人！");
        }
        else
        {
            Serial.println("无人！");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    vTaskDelete(NULL);
}