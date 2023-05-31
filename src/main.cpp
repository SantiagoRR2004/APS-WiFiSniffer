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





void setup() {
  // put your setup code here, to run once:
  M5.begin();
  
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(MAGENTA);
  M5.Lcd.setRotation(1);
  M5.Lcd.print("Hello world!");
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
