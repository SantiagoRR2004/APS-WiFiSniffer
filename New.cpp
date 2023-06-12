/**
   A M5StickC WiFi promiscuous mode packet sniffer by S. Marano, lazy adaptation of L. Podkalicki packet monitor.

   Link: https://github.com/SMH17/M5StackWiFiSniffer
*/
// vTaskDelay

#include <M5StickCPlus.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

// Configuración de la red WiFi
const char* ssid = "XXXXXXXXXXXXXX";
const char* password = "XXXXXXXXXXXXXXXX";

// Configuración del servidor MQTT
const char* mqttServer = "aps2023.is-a-student.com";
const int mqttPort = 1883;
const char* mqttClientId = "m5stick";
int j = 1;

// Intervalo de tiempo para el escaneo y envío de datos (en milisegundos)
// Creo que ya no lo uso
const unsigned long interval = 5000;  // 5 segundos

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Array para guardar MACs y RSIIs
char data[10000];
int i = 0; 

bool enablePacketHandling = true;

#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)

#define WIFI_CHANNEL_MAX               (13)

unsigned long tiempoEspera = WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS;
unsigned long tiempoInicio = 0;


uint8_t level = 0, channel = 1;


static wifi_country_t wifi_country = {.cc = "CN", .schan = 1, .nchan = 13};


typedef struct {

  unsigned frame_ctrl: 16;

  unsigned duration_id: 16;

  uint8_t addr1[6]; /* receiver address */

  uint8_t addr2[6]; /* sender address */

  uint8_t addr3[6]; /* filtering address */

  unsigned sequence_ctrl: 16;

  uint8_t addr4[6]; /* optional */

} wifi_ieee80211_mac_hdr_t;



typedef struct {

  wifi_ieee80211_mac_hdr_t hdr;

  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */

} wifi_ieee80211_packet_t;



static esp_err_t event_handler(void *ctx, system_event_t *event);

static void wifi_sniffer_init(void);

static void wifi_sniffer_set_channel(uint8_t channel);

static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);

static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);



esp_err_t event_handler(void *ctx, system_event_t *event) {

  return ESP_OK;

}


void wifi_sniffer_init(void) {

  tcpip_adapter_init();

  ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */

  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );

  ESP_ERROR_CHECK( esp_wifi_start() );

  esp_wifi_set_promiscuous(true);

  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

}


void connectToWiFi() {
  if (!WiFi.isConnected()) {
    WiFi.begin(ssid, password);

    while (!WiFi.isConnected()) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else{
    Serial.println("WiFi already connected");
  }
}


void connectToMQTT() {
  mqttClient.setServer(mqttServer, mqttPort);
  
  while (!mqttClient.connected()) {
    if (mqttClient.connect(mqttClientId, "aps_grupo_q", "EuMCYrjE")) {
      Serial.println("MQTT connected");
    } else {
      Serial.print("MQTT connection failed, rc= ");
      Serial.print(mqttClient.state());
      Serial.println("Retrying...");
      delay(2000);
    }
  }
}

void wifi_sniffer_set_channel(uint8_t channel) {

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

}

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type) {

  switch (type) {

    case WIFI_PKT_MGMT: return "MGMT";

    case WIFI_PKT_DATA: return "DATA";

    default:

    case WIFI_PKT_MISC: return "MISC";

  }

}


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

// the setup function runs once when you press reset or power the board

void setup() {
  
  // Initialize the M5StickC object
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(0, 128);
  // LCD display print
  M5.Lcd.printf("HOLA:%2.1f%%", 2.3);
  M5.Lcd.print("Wifi Sniffer\n Init...\n\n");
  
  // To allow print on serial monitor
  Serial.begin(115200);
  
  delay(10);

  // Start sniffer

  delay(5000);
  wifi_sniffer_init();

  // LCD display print
  M5.Lcd.print("Wifi Sniffer\n Running...");
  
}

// the loop function runs over and over again forever

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