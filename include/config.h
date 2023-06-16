#ifndef CONFIG_H
#define CONFIG_H

#define STRING_LEN 128
//-- datos wifi ap configuración
const char apWifiName[] = "XXXXXXXXXXXXXXXXXXXXXX";
const char wifiInitialApPassword[] = "XXXXXXXXXXXXXXXXXXXXXX";
//-- VARIABLES DE LOS PARAMETROS A CONFIGURAR
char mqttServerValue[STRING_LEN] = "aps2023.is-a-student.com";
const int mqttServerPortValue = 2883;
char mqttUserNameValue[STRING_LEN] = "admin";
char mqttUserPasswordValue[STRING_LEN] = "admin";
char mqttTopicValue[STRING_LEN] = "aps2023/Proyecto7";
char mqttAutoSend[STRING_LEN];
//-- CONFIGURACIÓN SERIAL PORT BASCULA
char serial1Baudios[STRING_LEN] = "9600";
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

#endif