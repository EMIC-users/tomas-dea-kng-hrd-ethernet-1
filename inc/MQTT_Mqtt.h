#ifndef MQTT_Mqtt_H_
#define MQTT_Mqtt_H_

#include <stdint.h>

#ifndef persist_
#define _PERSIST_STR2(x) #x
#define _PERSIST_STR(x) _PERSIST_STR2(x)
#define persist_ __attribute__((section(".persist." _PERSIST_STR(__COUNTER__))))
#endif
extern char MQTT_Mqtt_ip[16];
extern char MQTT_Mqtt_mac[18];
extern char MQTT_Mqtt_brokerIp[16];
extern char MQTT_Mqtt_netmask[16];
extern char MQTT_Mqtt_gateway[16];
extern uint16_t MQTT_Mqtt_brokerPort;
extern char MQTT_Mqtt_clientId[24];
extern char MQTT_Mqtt_username[32];
extern char MQTT_Mqtt_password[32];

void MQTT_Mqtt_init(void);

void MQTT_Mqtt_poll(void);

void MQTT_Mqtt_publish(char* topic, char* payload);



extern void MQTT_Mqtt_onConnected(void);


#endif

