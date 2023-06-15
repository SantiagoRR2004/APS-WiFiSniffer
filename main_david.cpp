#include <M5StickCPlus.h>   // M5StickC Plus library for the ESP32-based development board
#include "esp_wifi.h"       // ESP32 WiFi library
#include "esp_wifi_types.h" // ESP32 WiFi types
#include "esp_system.h"     // ESP32 system library
#include "esp_event.h"      // ESP32 event library
#include "esp_event_loop.h" // ESP32 event loop library
#include <ArduinoJson.h>


// Constants
#define WIFI_CHANNEL_SWITCH_INTERVAL (15000) // Interval between channel switches (in milliseconds)
#define WIFI_CHANNEL_MAX (13)                // Maximum WiFi channel number
#define MAX_MAC_ADDRESSES (250)

static uint8_t numAddresses = 0;
static char macAddresses[MAX_MAC_ADDRESSES][18];  // Array to store MAC addresses
static uint8_t channelNumbers[MAX_MAC_ADDRESSES]; // Array to store channel numbers
static int8_t signalLevels[MAX_MAC_ADDRESSES];    // Array to store signal levels

DynamicJsonDocument jsonDocument(JSON_OBJECT_SIZE(1) + MAX_MAC_ADDRESSES * JSON_OBJECT_SIZE(3));

uint8_t level = 0, channel = 1;

static wifi_country_t wifi_country = {.cc = "ES", .schan = 1, .nchan = 13}; // Set the country code to Spain

// Data structures for parsing WiFi packets
typedef struct
{
  unsigned frame_ctrl : 16;    // Frame control field
  unsigned duration_id : 16;   // Duration/ID field
  uint8_t addr1[6];            // MAC address 1
  uint8_t addr2[6];            // MAC address 2
  uint8_t addr3[6];            // MAC address 3
  unsigned sequence_ctrl : 16; // Sequence control field
  uint8_t addr4[6];            // MAC address 4
} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;

typedef struct
{
  wifi_ieee80211_mac_hdr_t hdr; // MAC header
  uint8_t payload[0];           // Packet payload
} __attribute__((packed)) wifi_ieee80211_packet_t;

// Event handler for system events
static esp_err_t event_handler(void *ctx, system_event_t *event);

// Function to initialize WiFi sniffer
static void wifi_sniffer_init(void);

// Function to set WiFi channel for sniffing
static void wifi_sniffer_set_channel(uint8_t channel);

// Function to convert WiFi packet type to string
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);

// Callback function for WiFi packet handling
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

// Event handler for system events
esp_err_t event_handler(void *ctx, system_event_t *event)
{
  return ESP_OK;
}

// Function to initialize WiFi sniffer
void wifi_sniffer_init(void)
{
  tcpip_adapter_init();                                      // Initialize the TCP/IP adapter
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL)); // Initialize the event loop
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));                        // Initialize the WiFi driver
  ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country));        // Set the WiFi country to Spain
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));     // Set WiFi storage to RAM
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));          // Set WiFi mode to NULL mode
  ESP_ERROR_CHECK(esp_wifi_start());                           // Start WiFi
  esp_wifi_set_promiscuous(true);                              // Enable promiscuous mode
  esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler); // Set the callback for WiFi packet handling
}



// Function to set WiFi channel for sniffing
void wifi_sniffer_set_channel(uint8_t channel)
{
  numAddresses = 0;                              // Clear the number of addresses
  memset(macAddresses, 0, sizeof(macAddresses)); // Clear the list of mac addresses

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE); // Set the WiFi channel for sniffing

  M5.Lcd.fillRect(0, 128, 300, 80, BLACK);
  M5.Lcd.setCursor(0, 128);
  M5.Lcd.printf("Current Channel: %d", channel); // Display current channel
}

// Function to convert WiFi packet type to string
const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch (type)
  {
  case WIFI_PKT_MGMT:
    return "MGMT";
  case WIFI_PKT_DATA:
    return "DATA";
  case WIFI_PKT_MISC:
    return "MISC";
  default:
    return "UNKNOWN";
  }
}

// Callback function for WiFi packet handling
void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;            // Get the WiFi packet
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload; // Get the IEEE80211 packet

  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

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

  JsonArray macArray = jsonDocument.createNestedArray(); // FIXME
  JsonObject macObject = macArray.createNestedObject();
  macObject["mac"] = mac2;
  macObject["channel"] = ppkt->rx_ctrl.channel;
  macObject["rssi"] = ppkt->rx_ctrl.rssi;

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

void setup()
{
  M5.begin();    
  M5.Axp.ScreenBreath(15);
  // Initialize the M5StickC Plus
  M5.Lcd.setRotation(1);      // Set the LCD rotation
  M5.Lcd.fillScreen(BLACK);   // Fill the screen with black color
  M5.Lcd.setTextColor(GREEN); // Set the text color to green
  M5.Lcd.setTextSize(1);      // Set the text size

  Serial.begin(115200); // Initialize the Serial communication

  wifi_sniffer_init(); // Initialize the WiFi sniffer

  M5.Lcd.setCursor(0, 128);
  M5.Lcd.printf("Current Channel: %d", channel); // Display current channel
}

void loop()
{
  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
  channel++;
  if (channel > WIFI_CHANNEL_MAX)
  {
    channel = 1;
  }
  wifi_sniffer_set_channel(channel);
}
