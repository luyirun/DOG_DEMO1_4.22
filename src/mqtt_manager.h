#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#define MQTT_MAX_PACKET_SIZE 4096 // 进一步增大缓冲区

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <functional>

typedef std::function<void(const String &topic, const String &payload)> MQTTCallback;

class MQTTManager {
public:
    MQTTManager(const char* mqtt_server, 
               int mqtt_port,
               const char* client_id,
               const char* mqtt_user,
               const char* mqtt_pass,
               const char* ca_cert = nullptr);

    bool begin();
    bool publish(const String& topic, const String& payload);
    bool subscribe(const String& topic);
    bool unsubscribe(const String& topic);
    void setCallback(MQTTCallback callback);
    void loop();

    static const char* getCACert();

    bool sendMessage(const String& topic, const String& payload); // 发送消息

private:
    bool connect();

    // 配置参数
    const char* _mqtt_server;
    int _mqtt_port;
    const char* _client_id;
    const char* _mqtt_user;
    const char* _mqtt_pass;
    const char* _ca_cert;

    WiFiClientSecure _wifiClient;
    PubSubClient _mqttClient;
    MQTTCallback _userCallback;
};

#endif

