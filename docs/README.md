# Wifi Sniffer with M5StickC Plus

## Overview

This is a project for a subject called "Adquisición y Procesamiento de la Señal" (Signal Acquisition and Processing) that accounted for 40% of the final grade. Each team was a pair, but teams that had the same proyect could collaborate. So in the end this proyect was made by six people. The submission took place on Tuesday, the 20th of June of 2023, and earned a grade of 9.5 out of 10 points.

## Project Summary

For this project we had to choose between 8 different types of proyects. We chose the one that consisted in making a Wifi Sniffer with multiple M5StickC Plus. These devices would detect the MAC address of devices that had their Wifi on and send them to a MQTT server. The server would then store the data in a MariaDB database with phpMyAdmin inside HomeAssistant. The data would be displayed in a Grafana and InfluxDB dashboard. The communication from inside the HomeAssistant was made with NodeRed. Finally, if the number of MAC addresses detected was greater than a number, a Telegram notification and email would be sent. This repository is only the code for the M5StickC Plus.

## Make it work

To make the code work inside of the M5StickC Plus, you need to change the following lines of code in the PubSubClient library.

In the PubSubClient.h file from:

```c
#define MQTT_MAX_PACKET_SIZE 256

size_t buildHeader(uint8_t header, uint8_t* buf, uint16_t length);
```

to

```c
#define MQTT_MAX_PACKET_SIZE 8192

size_t buildHeader(uint8_t header, uint8_t* buf, uint64_t length);
```

And in the PubSubClient.cpp file from:

```c
size_t PubSubClient::buildHeader(uint8_t header, uint8_t* buf, uint16_t length) {

uint16_t len = length;
```

to

```c
size_t PubSubClient::buildHeader(uint8_t header, uint8_t* buf, uint64_t length) {

uint64_t len = length;
```
