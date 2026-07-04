#ifndef CONFIG_H
#define CONFIG_H

// ====== Flags de compilación ======
// Activa los Serial.print "ruidosos" de por-frame (LDR, FPS, hemorragia).
// Dejalo comentado en uso normal: consumen ciclos de CPU y llenan el buffer
// serial en cada vuelta de loop(). Descomentar solo para debug puntual.
//#define DEBUG_EYES

const char* ssid = "PEPPE_001";
const char* password = "POLITECNICO";

IPAddress local_IP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

// ====== Objetos globales (definidos en main.ino) ======
extern Adafruit_VL6180X sensor;
extern WebServer server;
extern WebSocketsServer webSocket;

// ====== Variables de conexión / estado general ======
extern std::vector<int> values;
extern std::map<int, int> valueFrequency;
extern bool sendingData;
extern int conexionesActivas;
extern String accionEnvio;
extern String globalEstudiante;
extern int globalDuracionPrueba;

// ====== Presiones (RCP) ======
extern unsigned long lastAverageTime;
extern const unsigned long averageInterval;
extern int cuentaPresiones;
extern float maxRelativo;
extern float minRelativo;
extern bool comprimiendo;
extern bool alcanzoMinimo;
extern uint8_t lecturaMaximaCMPresion;
extern uint8_t diferenciaCMPresion;

// ====== Ventilación ======
#define MAX_REGISTROS 50   // Tamaño máximo de arrays para almacenar datos
#define pinVentilacion 36  // Pin digital para el sensor YF-B1
#define pinBateria 34

extern unsigned long sumaDuracion;
extern unsigned long sumaVentilacion;
extern int espacioEntreVenti;
extern unsigned long ritmoVentilacion;
extern float flujoAire;
extern unsigned long lastTime;

extern volatile int pulseCount;
extern unsigned long lastFlowTime;
extern unsigned long flowDuration;
extern unsigned long lastEndTime;
extern unsigned long flowGap;

extern float frequency, flowRateL_min, flowRateL_Beath;
extern float calibrationFactor;

extern unsigned long duraciones[MAX_REGISTROS];
extern unsigned long ventilaciones[MAX_REGISTROS];
extern int indiceVentilacion;

extern int totalVentilaciones;
extern unsigned long totalTiempoEntreVentilaciones;
extern unsigned long totalDuracionVentilaciones;
extern float totalAireVentilado;

extern unsigned long lastStatsTime;
extern const unsigned long statsInterval;
extern bool enviarBateria;

// ====== Batería ======
extern const float R1, R2, Vmin, Vmax, VREF, alpha;
extern float filteredADC;
extern bool filtroInicializado;
extern unsigned long lastBatteryCheck;
extern const int batteryInterval;

#endif
