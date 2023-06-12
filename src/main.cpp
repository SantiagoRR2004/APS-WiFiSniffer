#include <M5StickCPlus.h>    // M5StickC Plus library for the ESP32-based development board
#include "esp_wifi.h"        // ESP32 WiFi library
#include "esp_wifi_types.h"  // ESP32 WiFi types
#include "esp_system.h"      // ESP32 system library
#include "esp_event.h"       // ESP32 event library
#include "esp_event_loop.h"  // ESP32 event loop library
#include <PubSubClient.h>
#include <WiFi.h>
#include <stdio.h>

// Constants
//Cada vez que se encuentran las cadenas se sustituyen por el número
#define WIFI_CHANNEL_SWITCH_INTERVAL  (20000) // Interval between channel switches (in milliseconds)
#define WIFI_CHANNEL_MAX               (13) // Maximum WiFi channel number
#define MAX_MAC_ADDRESSES (100)

// Configuración de la red WiFi
const char* ssid = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
const char* password = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

// Configuración del servidor MQTT
const char* mqttServer = "aps2023.is-a-student.com";
const int mqttPort = 1883;  // Default MQTT port is 1883
const char* mqttClientId = "m5stick";
int j = 1;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
static uint8_t numAddresses = 0;
static char macAddresses[MAX_MAC_ADDRESSES][18];  // Array to store MAC addresses
static uint8_t channelNumbers[MAX_MAC_ADDRESSES]; // Array to store channel numbers
static int8_t signalLevels[MAX_MAC_ADDRESSES];    // Array to store signal levels

uint8_t level = 0, channel = 1; // Crea 2 variables de un bit

static wifi_country_t wifi_country = {.cc = "CN", .schan = 1, .nchan = 13}; /** @brief Structure describing WiFi country-based regional restrictions. */

// Data structures for parsing WiFi packets
typedef struct { //Esto crea una estructura con la distinta información en su interior. Parecido a un objeto de una clase.

  unsigned frame_ctrl: 16;   // Frame control field
  unsigned duration_id: 16;  // Duration/ID field
  uint8_t addr1[6]; /* receiver address */  // MAC address 1
  uint8_t addr2[6]; /* sender address */    // MAC address 2
  uint8_t addr3[6]; /* filtering address */ // MAC address 3
  unsigned sequence_ctrl: 16;               // Sequence control field
  uint8_t addr4[6]; /* optional */          // MAC address 4

} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;


typedef struct { //Crea otra estructura.

  wifi_ieee80211_mac_hdr_t hdr; // MAC header
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */ // Packet payload

} __attribute__((packed)) wifi_ieee80211_packet_t;


// En el siguiente código se crean funciones pero no se definen lo que hacen

// Event handler for system events
static esp_err_t event_handler(void *ctx, system_event_t *event);
//event_handler function is expected to handle events of type system_event_t and return an esp_err_t error code. The ctx parameter can be used to pass additional context data

// Function to initialize WiFi sniffer
static void wifi_sniffer_init(void);
//wifi_sniffer_init function is responsible for initializing the Wi-Fi sniffer

// Function to set WiFi channel for sniffing
static void wifi_sniffer_set_channel(uint8_t channel);
//wifi_sniffer_set_channel function is responsible for setting the Wi-Fi channel to be used for sniffing

// Function to convert WiFi packet type to string
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
// wifi_sniffer_packet_type2str function is responsible for converting a wifi_promiscuous_pkt_type_t packet type to a string representation

// Callback function for WiFi packet handling
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);
//wifi_sniffer_packet_handler function is a callback function that handles incoming packets captured by a Wi-Fi sniffer. It takes a pointer to the packet data buffer (buff) and the packet type (type) as input parameters

// Ahora definimos lo que hacen las anteriores funciones

// Event handler for system events
esp_err_t event_handler(void *ctx, system_event_t *event) {
  return ESP_OK; //returns ESP_OK, which indicates that the event handling was successful
}

// Function to initialize WiFi sniffer
void wifi_sniffer_init(void) {

  tcpip_adapter_init();                                      // Initialize the TCP/IP adapter
  //This function initializes the TCP/IP adapter stack for network communication
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL)); // Initialize the event loop
  //This initializes the event loop and registers the event_handler function to handle system events.
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  // This initializes a Wi-Fi configuration structure with default values.
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));                        // Initialize the WiFi driver
  //This initializes the Wi-Fi driver with the provided configuration.
  ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country));        // Set the WiFi country to Spain
  /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));     // Set WiFi storage to RAM
  //This sets the Wi-Fi storage type to be used.
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));          // Set WiFi mode to NULL mode
  //his sets the Wi-Fi mode to WIFI_MODE_NULL, which means that the Wi-Fi interface is initialized but not connected to any network
  ESP_ERROR_CHECK(esp_wifi_start());                           // Start WiFi
  //This starts the Wi-Fi interface
  esp_wifi_set_promiscuous(true);                              // Enable promiscuous mode
  //This enables promiscuous mode on the Wi-Fi interface to capture all packets
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler); // Set the callback for WiFi packet handling
  //This sets the callback function wifi_sniffer_packet_handler to handle received packets in promiscuous mode

}

// Function to set WiFi channel for sniffing
void wifi_sniffer_set_channel(uint8_t channel) {

  numAddresses = 0;                              // Clear the number of addresses
  memset(macAddresses, 0, sizeof(macAddresses)); // Clear the list of mac addresses

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE); // Set the WiFi channel for sniffing
  //sets the Wi-Fi channel for sniffing

  M5.Lcd.fillRect(0, 128, 300, 80, BLACK);
  M5.Lcd.setCursor(0, 128);
  M5.Lcd.printf("Current Channel: %d", channel); // Display current channel

}

// Function to convert WiFi packet type to string
const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type) {

  switch (type) { //checks the packet type
    case WIFI_PKT_MGMT: return "MGMT";
    case WIFI_PKT_DATA: return "DATA";
    case WIFI_PKT_MISC: return "MISC";
    default:
    return "MISC";

  }
}

// Callback function for WiFi packet handling
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {

  if (type != WIFI_PKT_MGMT)
    // It checks if the packet type is not WIFI_PKT_MGMT
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;            // Get the WiFi packet
  //buff parameter to a pointer of type wifi_promiscuous_pkt_t* and assigns it to ppkt
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload; // Get the IEEE80211 packet
  //ppkt->payload to a pointer of type wifi_ieee80211_packet_t* and assigns it to ipkt. This provides access to the IEEE 802.11 packet structure
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  //a pointer hdr of type wifi_ieee80211_mac_hdr_t* and assigns the address of ipkt->hdr to it. This allows access to the MAC header of the packet

  char mac1[18];
  sprintf(mac1, "%02X:%02X:%02X:%02X:%02X:%02X", hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);

  char mac2[18];
  sprintf(mac2, "%02X:%02X:%02X:%02X:%02X:%02X", hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);

  /**/ // COMMENT THIS SECTION FOR NO FILTERING. NOT RECOMMENDED!!!! DEVICE RUNNING OUT OF MEMORY
  for (int i = 0; i < numAddresses; i++)
  {
    // if (strcmp(mac1, macAddresses[i]) == 0 || strcmp(mac2, macAddresses[i]) == 0) // STRICT FILTER. MOST PACKETS ARE LOST AND ONLY SOME USERS ARE SHOWN. MOST READABLE. FOR TESTING PURPOSES ONLY
    if (strcmp(mac2, macAddresses[i]) == 0) // SOFT FILTER. MOST PACKETS ARE LOST, BUT ALL CLIENTS ARE SHOWN. REOMMENDED MODE.
    {
      return; // Skip duplicate MAC addresses
    }
  }
  /**/ // NO FILTERING

  strcpy(macAddresses[numAddresses], mac1); // Store the RX MAC address
  numAddresses++;
  strcpy(macAddresses[numAddresses], mac2); // Store the TX MAC address
  numAddresses++;

  static int line = 0;
  M5.Lcd.setCursor(0, line * 8);
  M5.Lcd.printf("A1RX:%s CH:%d RSSI:%ddBm\n", mac1, ppkt->rx_ctrl.channel, ppkt->rx_ctrl.rssi); // Display MAC address, channel, and signal level
  line++;

  M5.Lcd.setCursor(0, line * 8);
  M5.Lcd.printf("A2TX:%s CH:%d RSSI:%ddBm\n", mac2, ppkt->rx_ctrl.channel, ppkt->rx_ctrl.rssi); // Display MAC address, channel, and signal level
  line++;

  if (line >= 16)
  {
    M5.Lcd.fillScreen(BLACK); // Clear the screen if it exceeds 16 lines
    M5.Lcd.fillRect(0, 128, 300, 80, BLACK);
    M5.Lcd.setCursor(0, 128);
    M5.Lcd.printf("Current Channel: %d", channel); // Display current channel
    line = 0;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming messages here
}

void setup() {
  // put your setup code here, to run once:
  M5.begin();                 // Initialize the M5StickC Plus
  M5.Lcd.setRotation(1);      // Set the LCD rotation
  M5.Lcd.fillScreen(BLACK);   // Fill the screen with black color
  M5.Lcd.setTextColor(GREEN); // Set the text color to green
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 128);

  Serial.begin(115200); // Initialize the Serial communication

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(WiFi.status()); // Needs to print 3
  }

  M5.Lcd.printf("");
  M5.Lcd.printf("WiFi connected");
  M5.Lcd.printf("IP address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

  wifi_sniffer_init(); // Initialize the WiFi sniffer

  // LCD display print
  M5.Lcd.printf("Current Channel: %d", channel); // Display current channel
}

void loop() {
  // put your main code here, to run repeatedly:

  mqttClient.loop();

  String message = "Hello, Mosquitto!"; // Your message here
  mqttClient.publish("topic", message.c_str());


  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS); //Tiempo de retraso
  channel++;
  if (channel > WIFI_CHANNEL_MAX)
  {
    channel = 1;
  }
  wifi_sniffer_set_channel(channel);
}

