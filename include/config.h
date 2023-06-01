#define STRING_LEN 128
//-- datos wifi ap configuración
const char apWifiName[] = "gateway_ha";
const char wifiInitialApPassword[] = "12345678";
//-- VARIABLES DE LOS PARAMETROS A CONFIGURAR
char mqttServerValue[STRING_LEN];
char mqttServerPortValue[STRING_LEN];
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];
char mqttTopicValue[STRING_LEN];
char mqttAutoSend[STRING_LEN];
//-- CONFIGURACIÓN SERIAL PORT BASCULA
char serial1Baudios[STRING_LEN];
char modeloBascula[STRING_LEN];
char udBascula[STRING_LEN];
//-- CONFIGURACIÓN IP FIJA AÑADIR A LA CONFIGURACION DE PARAMETROS
char staticIPValue[STRING_LEN];
char ipAddressValue[STRING_LEN];
char gatewayValue[STRING_LEN];
char netmaskValue[STRING_LEN];

char visualNotificationValue[STRING_LEN];
char delaySendingDataValue[STRING_LEN];

String MAC;
byte chipMAC[6]; 