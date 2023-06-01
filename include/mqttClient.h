#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoHA.h>


unsigned long lastMqttConnectionAttempt = 0;
WiFiClient mqttNet;
HADevice device(nullptr);
HAMqtt mqtt(mqttNet, device);
HASensor HAbascula("bascula",mqtt);
HASensor HAcounter("counter",mqtt);
HASensor HAIPAddress("ipaddress",mqtt);
HASensor HAModelo("modelo",mqtt);
HABinarySensor HAPing("ping",false,mqtt);
void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length) {
    Serial.print("New message on topic: ");
    Serial.println(topic);
    Serial.print("Data: ");
    Serial.println((const char*)payload);
    mqtt.publish("myPublishTopic", "hello");
}

void onMqttConnected() {
    Serial.println("Connected to the broker!");
    // You can subscribe to custom topic if you need
    mqtt.subscribe("myCustomTopic");
}

void onMqttConnectionFailed() {
    Serial.println("Failed to connect to the broker!");
}

void SetupMqtt(){
    // set device's details (optional)
    uint64_t chipid;
    chipid = ESP.getEfuseMac();
    byte v_id[6];
    for (int i=0;i<6;i++){
        v_id[i]=(byte)((chipid&((uint64_t)0xFF<<(8*i)))>>(8*i));
    }
    device.setUniqueId(v_id,sizeof(v_id));
    device.enableLastWill();
    device.enableSharedAvailability();
   // String name="Bascula"+String(mqttTopicValue);
   // device.setName(name.c_str());
    device.setName("Bascula StampM5");
    device.setSoftwareVersion("1.0.0");
    device.setManufacturer("Innebo Ingenieria S.L.");
    device.setModel("RS232");
    // configure sensor (optional)
     
    HAbascula.setUnitOfMeasurement("kg");
    HAbascula.setDeviceClass("temperature");
    HAbascula.setIcon("mdi:weight");
    HAbascula.setName("Bascula peso");

    HAcounter.setUnitOfMeasurement("ud");
    HAcounter.setDeviceClass("energy");
    HAcounter.setIcon("mdi:counter");
    HAcounter.setName("Bascula contador");
      
    HAIPAddress.setDeviceClass("energy");
    HAIPAddress.setIcon("mdi:ip-network-outline");//mdi:ip-network-outline
    HAIPAddress.setName("IP Address");    

    HAModelo.setDeviceClass("energy");
    HAModelo.setIcon("mdi:slot-machine");
    HAModelo.setName("Modelo"); 

    HAPing.setName("ping");

    mqtt.onMessage(onMqttMessage);
    mqtt.onConnected(onMqttConnected);
    mqtt.onConnectionFailed(onMqttConnectionFailed);
}

bool connectMqttOptions()
{
  bool result;
  int port=String(mqttServerPortValue).toInt();
  if (mqtt.isConnected()) return true;
  if (mqttUserPasswordValue[0] != '\0')
  {
    result = mqtt.begin(mqttServerValue,port,mqttUserNameValue,mqttUserPasswordValue);
  }
  else
  {
    result = mqtt.begin(mqttServerValue,port);
  }
  return result;
}

bool connectMqtt()
{
  unsigned long now = millis();
  if (1000 > now - lastMqttConnectionAttempt)
  {
    // Do not repeat within 1 sec.
    return false;
  }
  Serial.println("Conectando al servidor MQTT...");
  if (!connectMqttOptions())
  {
    lastMqttConnectionAttempt = now;
    return false;
  }
  Serial.println("Conectada!");
  Serial.println("Host:"+String(mqttServerValue)+":"+String(mqttServerPortValue));
  return true;
}
void callback(char* topic, byte* payload, unsigned int length) {

}


