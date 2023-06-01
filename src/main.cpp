#include <M5StickCPlus.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#define WIFI_CHANNEL_SWITCH_INTERVAL  (500) //Cada vez que se encuentra esta cadena se sustituye por el número
#define WIFI_CHANNEL_MAX               (13) //Lo mismo aquí

uint8_t level = 0, channel = 1; // Crea 2 variables de un bit

static wifi_country_t wifi_country = {.cc = "CN", .schan = 1, .nchan = 13}; /** @brief Structure describing WiFi country-based regional restrictions. */

typedef struct { //Esto crea una estructura con la distinta información en su interior. Parecido a un objeto de una clase.

  unsigned frame_ctrl: 16;
  unsigned duration_id: 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl: 16;
  uint8_t addr4[6]; /* optional */

} wifi_ieee80211_mac_hdr_t;


typedef struct { //Crea otra estructura.

  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */

} wifi_ieee80211_packet_t;


// En el siguiente código se crean funciones pero no se definen lo que hacen

static esp_err_t event_handler(void *ctx, system_event_t *event);
//event_handler function is expected to handle events of type system_event_t and return an esp_err_t error code. The ctx parameter can be used to pass additional context data
static void wifi_sniffer_init(void);
//wifi_sniffer_init function is responsible for initializing the Wi-Fi sniffer
static void wifi_sniffer_set_channel(uint8_t channel);
//wifi_sniffer_set_channel function is responsible for setting the Wi-Fi channel to be used for sniffing
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
// wifi_sniffer_packet_type2str function is responsible for converting a wifi_promiscuous_pkt_type_t packet type to a string representation
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);
//wifi_sniffer_packet_handler function is a callback function that handles incoming packets captured by a Wi-Fi sniffer. It takes a pointer to the packet data buffer (buff) and the packet type (type) as input parameters

// Ahora definimos lo que hacen las anteriores funciones

esp_err_t event_handler(void *ctx, system_event_t *event) {
  return ESP_OK; //returns ESP_OK, which indicates that the event handling was successful
}

void wifi_sniffer_init(void) {

  tcpip_adapter_init();
  //This function initializes the TCP/IP adapter stack for network communication
  ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  //This initializes the event loop and registers the event_handler function to handle system events.
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  // This initializes a Wi-Fi configuration structure with default values.
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  //This initializes the Wi-Fi driver with the provided configuration.
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  //This sets the country code for the Wi-Fi channel range.
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  //This sets the Wi-Fi storage type to be used.
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
  //his sets the Wi-Fi mode to WIFI_MODE_NULL, which means that the Wi-Fi interface is initialized but not connected to any network
  ESP_ERROR_CHECK( esp_wifi_start() );
  //This starts the Wi-Fi interface
  esp_wifi_set_promiscuous(true);
  //This enables promiscuous mode on the Wi-Fi interface to capture all packets
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
  //This sets the callback function wifi_sniffer_packet_handler to handle received packets in promiscuous mode

}

void wifi_sniffer_set_channel(uint8_t channel) {

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  //sets the Wi-Fi channel for sniffing
}

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type) {

  switch (type) { //checks the packet type
    case WIFI_PKT_MGMT: return "MGMT";
    case WIFI_PKT_DATA: return "DATA";

    default:
    case WIFI_PKT_MISC: return "MISC";
    //if the value is not recognized, the function returns the string "MISC"

  }
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {

  if (type != WIFI_PKT_MGMT)
    // It checks if the packet type is not WIFI_PKT_MGMT
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  //buff parameter to a pointer of type wifi_promiscuous_pkt_t* and assigns it to ppkt
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  //ppkt->payload to a pointer of type wifi_ieee80211_packet_t* and assigns it to ipkt. This provides access to the IEEE 802.11 packet structure
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  //a pointer hdr of type wifi_ieee80211_mac_hdr_t* and assigns the address of ipkt->hdr to it. This allows access to the MAC header of the packet

  // Print captured packets on serial monitor
  Serial.printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d,"
         " ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
         " ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
         " ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",

         wifi_sniffer_packet_type2str(type),

         ppkt->rx_ctrl.channel,
         ppkt->rx_ctrl.rssi,

         /* ADDR1 */
         hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
         hdr->addr1[3], hdr->addr1[4], hdr->addr1[5],

         /* ADDR2 */
         hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
         hdr->addr2[3], hdr->addr2[4], hdr->addr2[5],

         /* ADDR3 */
         hdr->addr3[0], hdr->addr3[1], hdr->addr3[2],
         hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]

        );
}


void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(0, 0);
  // LCD display print
  M5.Lcd.printf("HOLA:%2.1f%%", 2.3);
  M5.Lcd.print("Wifi Sniffer\n Init...\n\n");
  
  // To allow print on serial monitor
  Serial.begin(115200);

}

void loop() {
  // put your main code here, to run repeatedly:

  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS); //Tiempo de retraso
  wifi_sniffer_set_channel(channel);
  channel = (channel % WIFI_CHANNEL_MAX) + 1; //Encontramos el siguiente canal

  M5.Lcd.print(channel);

}

