Cambios que hay que hacer al PubSubClient.h

#define MQTT_MAX_PACKET_SIZE 256
#define MQTT_MAX_PACKET_SIZE 8192

size_t buildHeader(uint8_t header, uint8_t* buf, uint16_t length);
size_t buildHeader(uint8_t header, uint8_t* buf, uint64_t length);


Cambios que hay que hacer al PubSubClient.cpp

size_t PubSubClient::buildHeader(uint8_t header, uint8_t* buf, uint16_t length) {
size_t PubSubClient::buildHeader(uint8_t header, uint8_t* buf, uint64_t length) {

uint16_t len = length;
uint64_t len = length;

Enlace con explicación intercativa al código:

https://chat.openai.com/share/1bfb9632-3820-4c90-8d0d-3ca2054844f9