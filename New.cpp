/**
   A M5StickC WiFi promiscuous mode packet sniffer by S. Marano, lazy adaptation of L. Podkalicki packet monitor.

   Link: https://github.com/SMH17/M5StackWiFiSniffer
*/
// vTaskDelay


void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {

  if (enablePacketHandling) {

    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;

    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    // Print captured packets on serial monitor
    // Serial.printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d,"
  //
    //       " ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
  //
    //       " ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
  //
    //       " ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
  //
    //       wifi_sniffer_packet_type2str(type),
  //
    //       ppkt->rx_ctrl.channel,
  //
    //       ppkt->rx_ctrl.rssi,
  //
    //       /* ADDR1 */
  //
    //       hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
  //
    //       hdr->addr1[3], hdr->addr1[4], hdr->addr1[5],

      //     /* ADDR2 */

        //   hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],

          // hdr->addr2[3], hdr->addr2[4], hdr->addr2[5],

        //  /* ADDR3 */

          //hdr->addr3[0], hdr->addr3[1], hdr->addr3[2],

          // hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]

        // );
    
    char macAddress[18];
    sprintf(macAddress, "%02X:%02X:%02X:%02X:%02X:%02X", hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
    
    int rssi = ppkt->rx_ctrl.rssi;
    
    char payload[100];
    sprintf(payload, "{\"mac\":\"%s\",\"rssi\":%d}", macAddress, rssi);

    if (strlen(data) != 0) {
      strcat(data, ",");
    }

    //strcat(data, payload);
    strcpy(data, payload);

    i = i+1;
    
    Serial.printf("MAC: %s, RSSI: %d\n", macAddress, rssi);
    Serial.println(i);
  }
}



void loop() {


  if (i >= 10){
    Serial.println("Entré al if");
    // Booleano para que si encuentra un paquete no haga nada hasta tener la configuración necesaria.
    enablePacketHandling = false;
    //Desactivar la configuración anterior para poder concectarse a MQTT
    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    Serial.println("Procedo a conectar");

    connectToWiFi();
    connectToMQTT();

    Serial.println("Hasta aquí llego");
    char mqttTopic[30];
    sprintf(mqttTopic, "aps2023/GrupoQ/datos%d", j);
    j = j+1;

    mqttClient.publish(mqttTopic, data);
    Serial.println(data);
    
    char data[1000];
    Serial.println(data);
    i = 0;

    delay(5000);
    Serial.println(data);
    // Start sniffer again
    tcpip_adapter_init();

    cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */

    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );

    ESP_ERROR_CHECK( esp_wifi_start() );

    esp_wifi_set_promiscuous(true);
    
    enablePacketHandling = true;

  }

  // Comprobar el temporizador para el retraso
  unsigned long tiempoActual = xTaskGetTickCount();
  if (tiempoActual - tiempoInicio >= tiempoEspera) {
    // Acciones a realizar después de transcurrido el tiempo de espera
    wifi_sniffer_set_channel(channel);

    channel = (channel % WIFI_CHANNEL_MAX) + 1;
    
    // Reiniciar el temporizador
    tiempoInicio = xTaskGetTickCount();
  }
}