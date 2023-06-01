#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#include <IotWebConfESP32HTTPUpdateServer.h>

// -- key configuración específica. Si se cambia la estructura + o - parametros cambiar este valor para actualizar
#define CONFIG_VERSION "v2.0"
// -- pin para entrar en modo configuración inicial, por ejemplo si se perdió la password
#define CONFIG_PIN G39
// -- pin que indica el estado. LOw al arrancar, parpadeo conectando a wifi, HIGH (apagado led?) al conectar wifi
#define STATUS_PIN G10

bool needReset = false;
bool needMqttConnect = false;

//-- SERVIDORES PARA  web config
DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;

static char chooserBaudValues[][STRING_LEN] = {"1200", "2400", "4800", "9600", "14400", "19200"};
static char chooserBaudNames[][STRING_LEN] = {"1200", "2400", "4800", "9600", "14400", "19200"};
static char choosermodeloBasculaValues[][STRING_LEN] = {"bscRawData", "bscBizerba", "bscBR16", "bscVarpe"};
static char choosermodeloBasculaNames[][STRING_LEN] = {"RawData", "Bizerba", "BR16", "Varpe"};
static char chooserBasculaUdValues[][STRING_LEN] = {"udkg", "udgr"};
static char chooserBasculaUdNames[][STRING_LEN] = {"kg", "gr"};
//-- ESTRUCTURA CONFIGURACION
IotWebConf iotWebConf(apWifiName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

// -- You can also use namespace formats e.g.: iotwebconf::ParameterGroup
IotWebConfParameterGroup connGroup = IotWebConfParameterGroup("conn", "Parámetros de conexión");
IotWebConfCheckboxParameter staticIParam = IotWebConfCheckboxParameter("IP Estática", "dhcpDisabled", staticIPValue, STRING_LEN);
IotWebConfTextParameter ipAddressParam = IotWebConfTextParameter("IP address", "ipAddress", ipAddressValue, STRING_LEN, "text", nullptr, "192.168.3.222");
IotWebConfTextParameter gatewayParam = IotWebConfTextParameter("Gateway", "gateway", gatewayValue, STRING_LEN, "text", nullptr, "192.168.3.0");
IotWebConfTextParameter netmaskParam = IotWebConfTextParameter("Subnet mask", "netmask", netmaskValue, STRING_LEN, "text", nullptr, "255.255.255.0"); // mqttAutoSend

// -- You can also use namespace formats e.g.: iotwebconf::ParameterGroup
IotWebConfParameterGroup mqttGroup = IotWebConfParameterGroup("mqtt", "MQTT-configuración");
IotWebConfTextParameter mqttServerParam = IotWebConfTextParameter("Servidor ", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfTextParameter mqttServerPortParam = IotWebConfNumberParameter("Puerto ", "mqttServerPort", mqttServerPortValue, STRING_LEN);
IotWebConfTextParameter mqttUserNameParam = IotWebConfTextParameter("Usuario", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfPasswordParameter mqttUserPasswordParam = IotWebConfPasswordParameter("Password", "mqttPass", mqttUserPasswordValue, STRING_LEN);
IotWebConfTextParameter mqttTopicParam = IotWebConfTextParameter("Nombre", "mqttTopic", mqttTopicValue, STRING_LEN);
IotWebConfCheckboxParameter mqttAutoSendParam = IotWebConfCheckboxParameter("Envío automático", "mqttAutosend", mqttAutoSend, STRING_LEN);
// -- You can also use namespace formats e.g.: i

IotWebConfParameterGroup serialGroup = IotWebConfParameterGroup("serial", "Parametro bascula."); // modeloBascula
IotWebConfSelectParameter serialBaudiosParam = IotWebConfSelectParameter("Baudios", "serialBaud", serial1Baudios, STRING_LEN, (char *)chooserBaudValues, (char *)chooserBaudNames, sizeof(chooserBaudNames) / STRING_LEN, STRING_LEN);
IotWebConfSelectParameter moduloBasculaParam = IotWebConfSelectParameter("Modelo báscula", "modeloBascula", modeloBascula, STRING_LEN, (char *)choosermodeloBasculaValues, (char *)choosermodeloBasculaNames, sizeof(choosermodeloBasculaNames) / STRING_LEN, STRING_LEN);
//IotWebConfSelectParameter moduloBasculaUdParam = IotWebConfSelectParameter("Unidades báscula", "udBascula", udBascula, STRING_LEN, (char *)chooserBasculaUdValues, (char *)chooserBasculaUdNames, sizeof(chooserBasculaUdNames) / STRING_LEN, STRING_LEN);
IotWebConfCheckboxParameter visualNotificationParam = IotWebConfCheckboxParameter("Notificación visual", "visualNotification", visualNotificationValue, STRING_LEN);
IotWebConfNumberParameter delaySendingDataParam = IotWebConfNumberParameter("Retardo entre envios (ms)", "delaySendingData", delaySendingDataValue, STRING_LEN);
// -- CABECERAS FUNCIONES delaySendingDataValue
void createWebConfig();
void handleRoot();
void wifiConnected();
void configSaved();
bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper);
void connectWifi(const char *ssid, const char *password);

// -- INICIALIZA WEBCONFIG
void SetupWebConfig()
{
  createWebConfig();
  server.on("/", handleRoot);
  server.on("/config", []
            { iotWebConf.handleConfig(); });
  server.onNotFound([]()
                    { iotWebConf.handleNotFound(); });
  Serial.println("Ready.");
}

// -- CREA CONFIGURACIÓN INICIAL

void createWebConfig()
{
  // añade items a los grupos
  mqttGroup.addItem(&mqttServerParam);
  mqttGroup.addItem(&mqttServerPortParam);
  mqttGroup.addItem(&mqttUserNameParam);
  mqttGroup.addItem(&mqttUserPasswordParam);
  mqttGroup.addItem(&mqttTopicParam);
  mqttGroup.addItem(&mqttTopicParam);
  connGroup.addItem(&staticIParam);
  connGroup.addItem(&ipAddressParam);
  connGroup.addItem(&gatewayParam);
  connGroup.addItem(&netmaskParam);
  serialGroup.addItem(&serialBaudiosParam);
  serialGroup.addItem(&moduloBasculaParam);
 // serialGroup.addItem(&moduloBasculaUdParam);
  serialGroup.addItem(&visualNotificationParam);
  serialGroup.addItem(&mqttAutoSendParam);
  serialGroup.addItem(&delaySendingDataParam);
  // añade grupos a la web
  iotWebConf.addParameterGroup(&connGroup);
  iotWebConf.addParameterGroup(&mqttGroup);
  iotWebConf.addParameterGroup(&serialGroup);
  // parametros
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.setWifiConnectionHandler(&connectWifi);
  iotWebConf.getApTimeoutParameter()->visible = true;
  // -- Define how to handle updateServer calls.
  iotWebConf.setupUpdateServer(
      [](const char *updatePath)
      { httpUpdater.setup(&server, updatePath); },
      [](const char *userName, char *password)
      { httpUpdater.updateCredentials(userName, password); });
  // -- Initializing the configuration.
  bool validConfig = iotWebConf.init();
  if (!validConfig)
  {
    mqttServerValue[0] = '\0';
    mqttServerPortValue[0] = '\0';
    mqttUserNameValue[0] = '\0';
    mqttUserPasswordValue[0] = '\0';
    mqttTopicValue[0] = '\0';
    mqttAutoSend[0] = '\0';
    serial1Baudios[0] = '\0';
    modeloBascula[0] = '\0';
    udBascula[0] = '\0';
    staticIPValue[0] = '\0';
    ipAddressValue[0] = '\0';
    gatewayValue[0] = '\0';
    netmaskValue[0] = '\0';
    visualNotificationValue[0] = '\0';
    delaySendingDataValue[0] = '\0';
  }
}
int getModeloBascula()
{
  for (int i = 0; i < sizeof(choosermodeloBasculaNames) - 1; i++)
  {
    if (strncmp(choosermodeloBasculaValues[i], modeloBascula, sizeof(modeloBascula)) == 0)
      return i;
  }
  return -1;
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  // -- Let IotWebConf test and handle captive portal requests.
  //  if (!server.authenticate("admin", "admin"))
  //  {
  //   return server.requestAuthentication();
  //}
  iotWebConf.handleConfig();
}
// -- Let IotWebConf test and handle captive portal requests.

void connectWifi(const char *ssid, const char *password)
{
  needMqttConnect = true;
  Serial.println("conectada wifi");
  if (staticIParam.isChecked())
  {
    IPAddress ipAddress;
    IPAddress gateway;
    IPAddress netmask;
    ipAddress.fromString(String(ipAddressValue));
    netmask.fromString(String(netmaskValue));
    gateway.fromString(String(gatewayValue));

    if (!WiFi.config(ipAddress, gateway, netmask))
    {
      Serial.println("Error configurando ip estática");
    }
    Serial.print("ip: ");
    Serial.println(ipAddress);
    Serial.print("gw: ");
    Serial.println(gateway);
    Serial.print("net: ");
    Serial.println(netmask);
  }
  WiFi.begin(ssid, password);
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  needReset = true;
}

bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper)
{
  Serial.println("Validating form.");
  bool valid = true;
  Serial.println(mqttServerParam.getId());
  int l = webRequestWrapper->arg(mqttServerParam.getId()).length();
  if (l < 3)
  {
    mqttServerParam.errorMessage = "Proporciona al menos 2 caracteres!";
    valid = false;
  }

  return valid;
}
