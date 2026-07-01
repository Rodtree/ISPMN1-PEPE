#ifndef CONFIG_H
#define CONFIG_H


const char* ssid = "PEPPE_001";
const char* password = "POLITECNICO";

IPAddress local_IP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);
// Objetos globales
extern Adafruit_VL6180X sensor;
extern WebServer server;
extern WebSocketsServer webSocket;

// Variables globales
extern std::vector<int> values;
extern std::map<int, int> valueFrequency;
extern bool sendingData;
extern unsigned long lastAverageTime;
extern int cuentaPresiones;
extern unsigned long ritmoVentilacion;
extern volatile int pulseCount;
extern float flujoAire;
extern unsigned long lastTime;

#endif
