#include <M5StickCPlus.h>   // M5StickC Plus library for the ESP32-based development board
#include "esp_wifi.h"       // ESP32 WiFi library
#include "esp_wifi_types.h" // ESP32 WiFi types
#include "esp_system.h"     // ESP32 system library
#include "esp_event.h"      // ESP32 event library
#include "esp_event_loop.h" // ESP32 event loop library
#include "config.h"         // MQTT setup parameters and config
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_task_wdt.h" // WatchDog Library

// Constants
#define WIFI_CHANNEL_SWITCH_INTERVAL (10000) // Interval between channel switches (in milliseconds)
#define WIFI_CHANNEL_MAX (13)               // Maximum WiFi channel number
#define MAX_MAC_ADDRESSES (250)

static uint64_t chipId = ESP.getEfuseMac(); //The chip ID

#define WDT_TIMEOUT (59) // WatchDog Timeout in Seconds

//// MQTT CONSTANTS
char mqttRebootTopic[100];
const char *mqttRebootCommand = "reboot";

static uint8_t numAddresses = 0;
static char macAddresses[MAX_MAC_ADDRESSES][18] = {0};  // Array to store MAC addresses
static uint8_t channelNumbers[MAX_MAC_ADDRESSES] = {0}; // Array to store channel numbers
static int8_t signalLevels[MAX_MAC_ADDRESSES] = {0};    // Array to store signal levels

uint8_t level = 0, channel = 1;

bool lcdOn = false;

DynamicJsonDocument jsonDocument(JSON_OBJECT_SIZE(1) + MAX_MAC_ADDRESSES * JSON_OBJECT_SIZE(3));

WiFiClient espClient;
PubSubClient client(espClient);

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

// Callback function from MQTT
void handleMqttMessage(char *topic, byte *payload, unsigned int length);

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
  M5.Lcd.printf("Canal actual: %d      ID: %llu", channel, chipId); // Display current channel
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

  // Create a JSON object for the MAC address
  //JsonArray macArray = jsonDocument.createNestedArray();
  JsonObject macObject = jsonDocument.createNestedObject();
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
    M5.Lcd.printf("Canal actual: %d      ID: %llu", channel, chipId);// Display current channel
    line = 0;
  }
}

void start_Scan_Wifi()
{
  esp_wifi_stop();
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  wifi_sniffer_init();
}

void end_Scan_Wifi()
{
  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
}

bool setup_wifi()
{
  int retries = 0;
  const int maxRetries = 2; // Maximum number of connection retries

  // Connect to AP
  M5.Lcd.fillScreen(BLACK); // Fill the screen with black color
  M5.Lcd.setCursor(0, 8);
  esp_wifi_start();
  M5.Lcd.println("Conectando al AP...");
  M5.Lcd.println(apWifiName);

  while (retries <= maxRetries)
  {
    WiFi.begin(apWifiName, wifiInitialApPassword);
    uint32_t connectStartTime = millis();

    while (WiFi.status() != WL_CONNECTED && (millis() - connectStartTime) < 5000)
    {
      delay(750);
      M5.Lcd.print(".");
    }
    esp_task_wdt_add(NULL);
    if (WiFi.status() == WL_CONNECTED)
    {
      delay(1000);
      M5.Lcd.println("\nConectado al AP!");
      M5.Lcd.print("Direccion IP: ");
      M5.Lcd.print(WiFi.localIP());
      return true; // Connection successful, return true
    }
    else
    {
      M5.Lcd.println("\nConexion fallida. Reintentando...");
      retries++;
    }
  }

  M5.Lcd.println("\nConexion al AP fallida.");
  delay(2000);
  return false; // Connection failed, return false
}

void MQTT_Server()
{
  setup_wifi();
  client.setServer(mqttServerValue, mqttServerPortValue);
  client.connect(WiFi.macAddress().c_str(), mqttUserNameValue, mqttUserPasswordValue);
  M5.Lcd.println("\nIniciando MQTT...");
  if (client.connected())
  {
    M5.Lcd.println("MQTT Conectado");

    //// Remote Control
    client.setCallback(handleMqttMessage);
    client.subscribe(mqttRebootTopic, 1);
    delay(2000);
    String jsonString;
    serializeJson(jsonDocument, jsonString);
    // Serial.println(jsonString);
    client.setBufferSize(8192);
    char mqttTopic[100];
    sprintf(mqttTopic,"aps2023/Proyecto7/MAC_list/%llu",chipId);
    client.publish(mqttTopic, jsonString.c_str(),true);
    size_t jsonSize = measureJson(jsonDocument);
    // Print the size of the JSON object in bytes
    M5.Lcd.print("JSON (bytes): ");
    M5.Lcd.print(jsonSize);
    int numNestedObjects = jsonDocument.size();
    M5.Lcd.print("\nNumero de objetos: ");
    M5.Lcd.print(numNestedObjects);
    delay(1000);
    M5.Lcd.println("\n\nMQTT publicado!");
  }

  unsigned long tinicial;
  tinicial = millis();
  while ((millis() - tinicial) < 4000)
  {
    client.loop();
  }
  if ((millis() - tinicial) >= 4000)
  {
    client.disconnect();
    M5.Lcd.println("MQTT Desconectado!");
    delay(2000);
    M5.Lcd.fillScreen(BLACK);
  }
}

///////// CODE FOR THE REBOOT BUTTON
const int BUTTON_PIN = 37;      // GPIO pin number for the button
const int HOLD_DURATION = 3000; // Duration in milliseconds to consider a long press

unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;

void reboot()
{
  esp_restart();
}

void checkButton()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    // Button is pressed
    if (!buttonPressed)
    {
      buttonPressStartTime = millis();
      buttonPressed = true;
    }
  }
  else
  {
    // Button is released
    if (buttonPressed)
    {
      unsigned long buttonPressDuration = millis() - buttonPressStartTime;
      if (buttonPressDuration >= HOLD_DURATION)
      {
        reboot(); // Perform the reboot if the button was held for the specified duration
      }
      buttonPressed = false;
    }
  }
}
////////////////////////////////////

////// CODE FOR THE BACKLIGHT BUTTON

void toggleLcd()
{
  if (lcdOn)
  {
    M5.Axp.ScreenBreath(7); // Set the backlight to 0 to turn off the LCD screen
    lcdOn = false;
  }
  else
  {
    M5.Axp.ScreenBreath(15); // Set a non-zero backlight value to turn on the LCD screen
    lcdOn = true;
  }
}

////////////////////////////////

////// MQTT CALLBACK FUNTION

void handleMqttMessage(char *topic, uint8_t *payload, unsigned int length)
{
  // Convert the payload to a string
  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  // Check if the received topic is the reboot command topic
  if (String(topic) == mqttRebootTopic)
  {
    // Check if the received message is the reboot command
    if (message == mqttRebootCommand)
    {
      // Call the reboot function
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 8);
      M5.Lcd.println("Mensaje de control recibido!");
      M5.Lcd.println(message.c_str());
      delay(5000);
      reboot();
    }
  }
}

///////////////////

void setup()
{
  M5.begin();                           // Initialize the M5StickC Plus
  esp_task_wdt_init(WDT_TIMEOUT, true); // Initialize the Watchdog Timer
  M5.Axp.ScreenBreath(7);               // Set the backlight to 0 to turn off the LCD screen
  M5.Lcd.setRotation(1);                // Set the LCD rotation
  M5.Lcd.fillScreen(BLACK);             // Fill the screen with black color
  M5.Lcd.setTextColor(MAGENTA);           // Set the text color to green
  M5.Lcd.setTextSize(1);                // Set the text size

  // Serial.begin(115200); // Initialize the Serial communication

  pinMode(BUTTON_PIN, INPUT_PULLUP); // Initialize reboot button

  wifi_sniffer_init(); // Initialize the WiFi sniffer

  M5.Lcd.setCursor(0, 128);
  M5.Lcd.printf("Canal actual: %d      ID: %llu", channel, chipId); // Display current channel

  //We create the remote_control topic
  sprintf(mqttRebootTopic, "aps2023/Proyecto7/remote_control/%llu", chipId);

}

void loop()
{

  esp_task_wdt_reset(); // Reset the Watchdog Timer

  checkButton();

  M5.update();

  if (M5.BtnB.wasPressed())
  {
    toggleLcd(); // Call the toggleLcd() function when the secondary button is pressed
  }

  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
  channel++;

  if (channel > WIFI_CHANNEL_MAX)
  {
    // Serialize the JSON array to a string
    end_Scan_Wifi(); // Disable promiscuous mode;

    if (setup_wifi())
    {
      MQTT_Server();
    }

    M5.Lcd.fillScreen(BLACK);
    start_Scan_Wifi();

    channel = 1;
  }

  wifi_sniffer_set_channel(channel);
}
