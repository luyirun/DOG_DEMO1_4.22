#include "mqtt_manager.h"


// 证书定义
const char *ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

MQTTManager::MQTTManager(const char* mqtt_server,
                       int mqtt_port,
                       const char* client_id,
                       const char* mqtt_user,
                       const char* mqtt_pass,
                       const char* ca_cert)
    : _mqtt_server(mqtt_server),
      _mqtt_port(mqtt_port),
      _client_id(client_id),
      _mqtt_user(mqtt_user),
      _mqtt_pass(mqtt_pass),
      _ca_cert(ca_cert),
      _wifiClient(),
      _mqttClient(_wifiClient) {

    // 修复回调绑定方式
    _mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        if (_userCallback && length > 0) {
            payload[length] = '\0';
            _userCallback(String(topic), String((char*)payload));
        }
    });
}

bool MQTTManager::begin() {
    // 修复证书加载方式
    if(_ca_cert != nullptr){
        _wifiClient.setCACert(_ca_cert); 
        Serial.println("CA证书已设置");
    } else {
        Serial.println("警告: 未启用SSL验证");
        _wifiClient.setInsecure();
    }

    _mqttClient.setServer(_mqtt_server, _mqtt_port);
    _mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);
    return connect();
}

bool MQTTManager::connect() {
    if (_mqttClient.connected()) return true;

    Serial.println("\n=== MQTT连接 ===");
    Serial.printf("服务端: %s:%d\n", _mqtt_server, _mqtt_port);
    
    int retries = 3;
    while (retries-- > 0) {
        if (_mqttClient.connect(_client_id, _mqtt_user, _mqtt_pass)) {
            Serial.println("MQTT连接成功");
            return true;
        }
        Serial.printf("连接失败, 错误码: %d\n", _mqttClient.state());
        delay(2000);
    }
    return false;
}
//取消订阅
bool MQTTManager::unsubscribe(const String& topic) {
    if (!connect()) return false;
    return _mqttClient.unsubscribe(topic.c_str());
}

bool MQTTManager::publish(const String& topic, const String& payload) {
    if (!connect()) return false;
    return _mqttClient.publish(topic.c_str(), payload.c_str());
}

bool MQTTManager::subscribe(const String& topic) {
    if (!connect()) return false;
    
    Serial.printf("订阅主题: %s\n", topic.c_str());
    bool success = _mqttClient.subscribe(topic.c_str());
    if (!success) Serial.println("订阅失败");
    return success;
}

void MQTTManager::setCallback(MQTTCallback callback) {
    _userCallback = callback;
}

void MQTTManager::loop() {
    if (!_mqttClient.connected()) {
        connect();
    }
    _mqttClient.loop();
}

const char* MQTTManager::getCACert() {
    return ca_cert;
}

//同publish
bool MQTTManager::sendMessage(const String& topic, const String& payload) {
    return publish(topic, payload);
}
