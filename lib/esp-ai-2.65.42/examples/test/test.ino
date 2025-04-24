
/**
 * 本案例演示: 连接自定义服务和固定 wifi
 * 注意：配置值要用双引号！
 * 服务端使用：https://github.com/wangzongming/esp-ai/tree/master/example
 **/

#include <esp-ai.h>
ESP_AI esp_ai;

bool onBegin(){   
  return true;
  // Serial.println("没电了");
  // while (true) {
  //   esp_ai_dec.write(mei_dian_le, mei_dian_le_len);
  //   delay(8000);
  // }
  // return false;
}

void on_command(String command_id, String data) {
}
void onSessionStatus(String status) {
  // // 处理会话状态
  // if (status == "iat_start") {
  //   face->DoBlink();
  //   face->Behavior.Clear();
  //   face->Behavior.SetEmotion(eEmotions::Squint, 1.0);
  //   face->Update();
  // }
  // if (status == "iat_end") {
  //   face->DoBlink();
  //   face->Behavior.Clear();
  //   face->Behavior.SetEmotion(eEmotions::Happy, 1.0);
  //   face->Behavior.SetEmotion(eEmotions::Surprised, 1.0);
  //   face->Update();
  // }
  // if (status == "tts_real_end") {
  //   face->DoBlink();
  //   face->Behavior.Clear();
  //   face->Behavior.SetEmotion(eEmotions::Normal, 1.0);
  //   face->Update();
  // }
}


// 情绪检测
void onEmotion(String emotion){
  Serial.print("情绪： ");
  Serial.println(emotion);
}

void onPosition( String ip, String nation, String province, String city, String latitude, String longitude) {
    // 处理位置信息
    Serial.println("============ 定位成功 ============");
    Serial.println(ip);
    Serial.println(nation);
    Serial.println(province);
    Serial.println(city);
    Serial.println(latitude);
    Serial.println(longitude);
}
void setup() {
  Serial.begin(115200);

  Serial.print("===> 0可用内存: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println("KB");

  // [必  填] 是否调试模式， 会输出更多信息
  bool debug = true;
  // [必  填] wifi 配置： { wifi 账号， wifi 密码, "热点名字" } 可不设置，连不上wifi时会打开热点：ESP-AI，连接wifi后打开地址： 192.168.4.1 进行配网(控制台会输出地址，或者在ap回调中也能拿到信息)
  // ESP_AI_wifi_config wifi_config = { "", "", "ESP-AI" };
  // ESP_AI_wifi_config wifi_config = {"test", "12345678", "ESP-AI"};
  ESP_AI_wifi_config wifi_config = { "联域科技", "lykj987654321", "ESP-AI" };
  // [可 填] 服务配置： { 服务协议, 服务IP， 服务端口, "[可选] 请求参数" }
  // ESP_AI_server_config server_config = {  };
  ESP_AI_server_config server_config = { "http", "192.168.3.16", 8088 };
  // [必  填] 唤醒方案： { 方案, 语音唤醒用的阈值(本方案忽略即可), 引脚唤醒方案(本方案忽略), 发送的字符串 } 
  ESP_AI_wake_up_config wake_up_config = {"serial", 1, 10, "1"};
  // ESP_AI_wake_up_config wake_up_config = { "asrpro", 1, 10, "start" };
  // ESP_AI_wake_up_config wake_up_config = { "pin_high", 1, 10 };  // 如果按钮按下是低电平，那使用 pin_low 即可
  // ESP_AI_wake_up_config wake_up_config = { "pin_high_listen", 1, 10 };  // 按下对话
  // ESP_AI_wake_up_config wake_up_config = { "edge_impulse", 0.95, 10, "start",  5000, 1000  }; // 内置语音唤醒
  // ESP_AI_wake_up_config wake_up_config = { "diy" }; // 内置语音唤醒
  // ESP_AI_wake_up_config wake_up_config = {}; // 内置语音唤醒
  // wake_up_config.pin = 0;
  // strcpy(wake_up_config.wake_up_scheme, "pin_low");  // 唤醒方案


  /**
 * 语音唤醒方案：
 * edge_impulse：内置语音唤醒方案 (esp32S3板子支持)
 *       asrpro：天问语音模块唤醒
 *     pin_high：引脚高电平唤醒
 *      pin_low：引脚低电平唤醒
 *     pin_high_listen：引脚高电平聆听(按下对话)
 *      pin_low_listen：引脚低电平聆听(按下对话)
 *       serial：串口字符唤醒
 *       custom：自定义，自行调用 esp_ai.wakeUp() 唤醒
 */
  // ESP_AI_wake_up_config wake_up_config = {
  //   "pin_high",  // 唤醒方案
  //   0.95,        // 使用内置语音唤醒时的唤醒阈值 0-1
  //   10,          // 引脚唤醒方案时需要配置的引脚
  //   "start",     // 天问/串口唤醒方案时需要配置的对方发来的字符串
  //   1000         // vad 时间，默认 1500， 安静环境下非常有用。
  // };


  ESP_AI_volume_config volume_config = {
    .input_pin = 7,   // 输入引脚， 默认 7
    .max_val = 4096,  // 最大输出值
    .volume = 0.9,    // 默认音量
    .enable = false   // 启用电位器
  };


  // XIAO ESP32s3 麦克风配置
  ESP_AI_i2s_config_mic i2s_config_mic = { 5, 4, 6 };

  // XIAO ESP32s3 扬声器配置
  ESP_AI_i2s_config_speaker i2s_config_speaker = {
    .bck_io_num =15,  // BCK引脚
    .ws_io_num = 16,   // WS引脚
    .data_in_num = 7  // DATA输入引脚
  };

  // 错误监听, 需要放到 begin 前面，否则可能监听不到一些数据
  esp_ai.onBegin(onBegin);
  esp_ai.onPosition(onPosition);
  esp_ai.onEmotion(onEmotion);
  
  // // 第三代配置
  // esp_ai.begin({ debug, wifi_config, server_config, wake_up_config, volume_config, i2s_config_mic, i2s_config_speaker });

  // esp32s3 配置
  esp_ai.begin({ debug, wifi_config, server_config, wake_up_config, volume_config });
  // esp_ai.onEvent(on_command);

  // boot 按钮有问题，必须在 begen 后在从新设置一遍
  // pinMode(0, INPUT_PULLUP);
}


// long last_log_time = 0;

void loop() {
  // if (millis() - last_log_time > 3000) {
  //   last_log_time = millis();
  //   Serial.print("===> 可用内存: ");
  //   Serial.print(ESP.getFreeHeap() / 1024);
  //   Serial.println("KB");
  // }
  // Serial.print("   可用内存: ");
  // Serial.print(ESP.getFreeHeap() / 1024);
  // Serial.println("KB");
  // delay(100);

  esp_ai.loop();
}





// /***************************************************
//  *  表情调试程序
//  *
//  ****************************************************/

// // INCLUDES
// // Built-in Arduino I2C library
// #include <Wire.h>
// // Defines all face functionality
// #include "Face.h"

// // CONSTANTS
// const byte joystickPins[] = { 25, 26 };
// const byte blinkPin = 16;
// const byte moodPins[] = { 12, 13, 15 };
// const eEmotions moods[] = { eEmotions::Angry, eEmotions::Sad, eEmotions::Surprised };

// // GLOBALS
// Face *face;

// void setup(void) {
//   // Create a serial connection
//   Serial.begin(115200);
//   Serial.println(__FILE__ __DATE__);

//   // Configure input pins
//   pinMode(blinkPin, INPUT_PULLUP);

//   // Create a new face
//   face = new Face(/* screenWidth = */ 128, /* screenHeight = */ 64, /* eyeSize = */ 40, /* clock=*/38, /* data= */ 39);
//   // Assign the current expression
//   face->Expression.GoTo_Normal();

//   // Assign a weight to each emotion
//   //face->Behavior.SetEmotion(eEmotions::Normal, 1.0);
//   //face->Behavior.SetEmotion(eEmotions::Angry, 1.0);
//   //face->Behavior.SetEmotion(eEmotions::Sad, 1.0);
//   // Automatically switch between behaviours (selecting new behaviour randomly based on the weight assigned to each emotion)
//   face->RandomBehavior = true;

//   // Automatically blink
//   face->RandomBlink = true;
//   // Set blink rate
//   face->Blink.Timer.SetIntervalMillis(4000);

//   // Automatically choose a new random direction to look
//   face->RandomLook = false;

  
//   // 底部字幕
//   face->DrawStrAni("树晓晓: 你好，小明！我是来自未来的一个高级科技机器人。😊", false, 0);
   
// }

// float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
//   // Check if x is outside the input range
//   if (x <= in_min) return out_min;
//   if (x >= in_max) return out_max;

//   // Calculate the proportion of x relative to the input range
//   float proportion = (x - in_min) / (in_max - in_min);

//   // Map the proportion to the output range and return the result
//   return (proportion * (out_max - out_min)) + out_min;
// }

// void loop() {
//   // face->DrawStrAni("树晓晓: 你好，小明！我是来自未来的一个高级科技机器人。😊");
//   // delay(2000);
//   // face->DrawStrAni("你: 哈哈哈哈哈");
//   // delay(1000);

//   // static int lastMoveTime;

//   // // To avoid making eyes too twitchy (and to allow time for previous move animation to end),
//   // // only recalculate new position every 500ms
//   // if(millis() - lastMoveTime > 500) {
//   //   int yRaw = analogRead(25);
//   //   int xRaw = analogRead(26);
//   //   float y = mapFloat(yRaw, 0, 4095, 1.0, -1.0);
//   //   float x = mapFloat(xRaw, 0, 4095, -1.0, 1.0);
//   //   face->Look.LookAt(x, y);
//   //   lastMoveTime = millis();
//   // }
//   // // Blink when joystick pressed in
//   // if(!digitalRead(blinkPin)){
//   //   face->DoBlink();
//   // }

//   // // Use this code to set a particular emotion from a button
//   // for(int i=0; i<3; i++){
//   //   int32_t potValue = analogRead(moodPins[i]);
//   //   float moodLevel = map(potValue, 0, 4095, 0.0, 2.0);
//   //   face->Behavior.SetEmotion(moods[i], moodLevel);
//   // }


//   face->Behavior.SetEmotion(eEmotions::Angry, 1.0);

//   // 状态文字
//   face->DrawStrLeftTop("待命中", 1);
//   // face->Update();
//   // delay(2000);
//   // face->DrawStrLeftTop("聆听中", 1);

//   // // 底部字幕
//   // face->DrawStrAni("树晓晓: 你好，小明！我是来自未来的一个高级科技机器人。😊", false, 0);
  
//   // 绘制wifi图标
//   face->SetWifiStatus(true);
  
//   // 绘制静音图标
//   face->SetIsSilent(true);

//   // 绘制电量
//   face->SetKWH(80);

//   // Update the face!
//   face->Update();
//   delay(50);
// }