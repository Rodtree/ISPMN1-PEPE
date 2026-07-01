#include <Adafruit_VL6180X.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <map>
// Load TFT driver library
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft;           // A single instance is used for 1 or 2 displa
#include "config.h"
#include "ojo_config.h"  
Adafruit_VL6180X sensor;
WebServer server(80);
WebSocketsServer webSocket(81);

std::vector<int> values;
std::map<int, int> valueFrequency;
int conexionesActivas = 0; // Contador de conexiones activas
unsigned long sumaDuracion = 0;
unsigned long sumaVentilacion = 0;
int espacioEntreVenti = 2000; // en milis
bool sendingData = false;
String accionEnvio="";


/*----------- variables control de presiones -----*/
unsigned long lastAverageTime = 0;
const unsigned long averageInterval = 29000; // para el envio cada 30segundos de la cuenta de presiones
int cuentaPresiones = 0;
float maxPresion = 0.0;
float minPresion = 1000.0; // Un valor inicial alto para asegurar que cualquier lectura sea menor
// Variables para almacenar los valores máximos y mínimos relativos
float maxRelativo = 0; // Máximo relativo inicial (distancia máxima)
float minRelativo = 0; // Mínimo relativo inicial (distancia mínima)
bool comprimiendo = false; // Indica si se está realizando una compresión
unsigned long ultimoTiempoLectura = 0; // Tiempo de la última lectura
const unsigned long intervaloMinimo = 100; // Intervalo mínimo entre lecturas (en ms)
bool alcanzoMinimo = false; // Indica si se alcanzó un mínimo relativo


/*---------------------*/
unsigned long ritmoVentilacion = 0;
#define MAX_REGISTROS 50  // Tamaño máximo de arrays para almacenar datos
#define pinVentilacion 36  // Pin digital para el sensor YF-B1
#define pinBateria 34  

float flujoAire = 0.0;
unsigned long lastTime = 0;
String globalEstudiante;
int globalDuracionPrueba;
uint8_t diferenciaCMPresion;
uint8_t lecturaMaximaCMPresion;

volatile int pulseCount = 0;  // Contador de pulsos
unsigned long lastFlowTime = 0;  // Último tiempo de flujo detectado
unsigned long flowDuration = 0;  // Duración del flujo actual
unsigned long lastEndTime = 0;  // Tiempo en que terminó el último flujo
unsigned long flowGap = 0;  // Diferencia de tiempo entre flujos

float frequency, flowRateL_min, flowRateL_Beath;
float calibrationFactor = 7.5;  // Factor de conversión del sensor de ventilacion

// Arrays para almacenar valores mientras hay flujo
unsigned long duraciones[MAX_REGISTROS];
unsigned long ventilaciones[MAX_REGISTROS];
int indiceVentilacion = 0;  // Índice actual de almacenamiento

// Variables para estadísticas de ventilación
int totalVentilaciones = 0;
unsigned long totalTiempoEntreVentilaciones = 0;
unsigned long totalDuracionVentilaciones = 0;
float totalAireVentilado = 0.0;

// Temporizador para enviar estadísticas cada 30 segundos
unsigned long lastStatsTime = 0;
const unsigned long statsInterval = 30000; // 30 segundos
// solo para el envio de estado de la bateria al inicio
bool enviarBateria=true;
#include <serial_json_adapter.h>
void pulseCounter() {
    pulseCount++;  // Incrementar el contador de pulsos
    //Serial.println("Pulso detectado");  // Depuración
}

/*------------------*/
/*-------------------------------------*/
// === Configuración de batería ===
const float R1 = 22000.0;           // 22kΩ (entre batería y pin)
const float R2 = 12000.0;           // 12kΩ (entre pin y GND)
const float Vmin = 3.0;             // Voltaje mínimo (0%)
const float Vmax = 4.2;             // Voltaje máximo (100%)
const float VREF = 3.26;             // Valr medido entre 3v3 y gnd
const float alpha = 0.3;            // Factor de suavizado (0.1 = más liso, 0.9 = más rápido)
float filteredADC = 0;              // Variable para el filtro pasa-bajos
bool filtroInicializado = false;
unsigned long lastBatteryCheck = 0; // Control de tiempo
const int batteryInterval = 60000;   // Muestreo cada 60 segundos


/*-------------------------------------*/
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Iniciando ESP32...");
    //esp_task_wdt_init(500, false);
    //disableCore0WDT();
    //disableCore1WDT();
   
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ Error al montar SPIFFS");
        return;
    }
    Serial.println("✅ SPIFFS montado correctamente");

       esp_reset_reason_t reason = esp_reset_reason();
           Serial.print("Causa del reinicio: ");
    switch (reason) {
        case ESP_RST_POWERON: Serial.println("Encendido"); break;
        case ESP_RST_SW: Serial.println("Reinicio por software"); break;
        case ESP_RST_PANIC: Serial.println("Pánico (crash)"); break;
        case ESP_RST_INT_WDT: Serial.println("Watchdog interno"); break;
        case ESP_RST_TASK_WDT: Serial.println("Watchdog de tarea"); break;
        case ESP_RST_WDT: Serial.println("Watchdog general"); break;
        case ESP_RST_DEEPSLEEP: Serial.println("Despertar de deep sleep"); break;
        case ESP_RST_BROWNOUT: Serial.println("Brownout (caída de voltaje)"); break;
        case ESP_RST_SDIO: Serial.println("Reinicio por SDIO"); break;
        default: Serial.println("Desconocido"); break;
    }
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("❌ Error en WiFi.softAPConfig");
        return;
    }
    WiFi.softAP(ssid, password);
    Serial.println("✅ AP iniciado. IP: " + WiFi.softAPIP().toString());

    server.on("/", HTTP_GET, []() {
        File file = SPIFFS.open("/index.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Error al abrir index.html");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/panelEstudiante.html", HTTP_GET, []() {
        File file = SPIFFS.open("/panelEstudiante.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Error al abrir panelEstudiante.html");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/panelDocente.html", HTTP_GET, []() {
        File file = SPIFFS.open("/panelDocente.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Error al abrir panelDocente.html");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/generaInforme.html", HTTP_GET, []() {
        File file = SPIFFS.open("/generaInforme.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Error al abrir generaInforme.html");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/estilos.css", HTTP_GET, []() {
        File file = SPIFFS.open("/estilos.css", "r");
        if (!file) {
            Serial.println("❌ Error al abrir estilos.css");
            server.send(500, "text/plain", "Error al abrir estilos.css");
            return;
        } else {
            Serial.println("✅ estilos.css ok");
        }
        server.streamFile(file, "text/css");
        file.close();
    });

    server.on("/style2.css", HTTP_GET, []() {
        File file = SPIFFS.open("/style2.css", "r");
        if (!file) {
            Serial.println("❌ Error al abrir style2.css");
            server.send(500, "text/plain", "Error al abrir style2.css");
            return;
        } else {
            Serial.println("✅ style2.css ok");
        }
        server.streamFile(file, "text/css");
        file.close();
    });

    server.on("/chart.js", []() {
        File file = SPIFFS.open("/chart.js", "r");
        if (!file) {
            Serial.println("❌ Error al abrir libreria char.js");
            server.send(500, "text/plain", "Error al abrir libreria char.js");
            return;
        }
        Serial.println("✅ libreria char.js ok");
        server.streamFile(file, "application/js");
        file.close();
    });

    server.on("/timer.js", []() {
        File file = SPIFFS.open("/timer.js", "r");
        if (!file) {
            Serial.println("❌ Error al abrir libreria timer.js");
            server.send(500, "text/plain", "Error al abrir libreria timer.js");
            return;
        }
        Serial.println("✅ libreria char.js ok");
        server.streamFile(file, "application/js");
        file.close();
    });

    server.on("/chartjs_annotation_min.js", []() {
        File file = SPIFFS.open("/chartjs_annotation_min.js", "r");
        if (!file) {
            Serial.println("❌ Error al abrir libreria chartjs-plugin-annotation.min.js");
            server.send(500, "text/plain", "Error al abrir libreria chartjs-plugin-annotation.min.js");
            return;
        }
        Serial.println("✅ libreria chartjs-plugin-annotation.min.js ok");
        server.streamFile(file, "application/js");
        file.close();
    });

    server.on("/chartjs-plugin-datalabels.js", []() {
        File file = SPIFFS.open("/chartjs-plugin-datalabels.js", "r");
        if (!file) {
            Serial.println("❌ Error al abrir libreria chartjs-plugin-datalabels.js");
            server.send(500, "text/plain", "Error al abrir libreria chartjs-plugin-datalabels.js");
            return;
        }
        Serial.println("✅ libreria chartjs-plugin-datalabels.js ok");
        server.streamFile(file, "application/js");
        file.close();
    });
    // Calibración inicial del filtro (promedio de 50 lecturas)
        float sumaInicial = 0;
        for (int i = 0; i < 50; i++) {
         sumaInicial += analogRead(pinBateria);
         delay(10);
        }
        filteredADC = sumaInicial / 50;  // Inicializar con promedio{

        
    server.begin();
    Serial.println("✅ Servidor web iniciado en el puerto 80");

    webSocket.begin();
    webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
     switch (type) {
            case WStype_CONNECTED:
                if (conexionesActivas < 3) {
                    conexionesActivas++; // Incrementar el contador de conexiones activas
                    Serial.printf("🟢 Cliente %u conectado. Conexiones activas: %d\n", num, conexionesActivas);
                } else {
                    // Rechazar la conexión si ya hay 3 conexiones activas
                    webSocket.disconnect(num);
                    Serial.printf("🔴 Conexión rechazada. Límite de 3 conexiones alcanzado.\n");
                }
                break;

            case WStype_DISCONNECTED:
                if (conexionesActivas > 0) {
                    conexionesActivas--; // Decrementar el contador de conexiones activas
                }
                Serial.printf("🔴 Cliente %u desconectado. Conexiones activas: %d\n", num, conexionesActivas);
                break;

            case WStype_TEXT:
                Serial.printf("📩 Mensaje recibido: %s\n", payload);
                handleWebSocketMessage(payload, length);
                break;
            case WStype_ERROR:
                Serial.printf("WStype_ERROR", payload);
                handleWebSocketMessage(payload, length);
                break;
        }
    });
    Serial.println("✅ WebSockets iniciado en el puerto 81");

    pinMode(pinVentilacion, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pinVentilacion), pulseCounter, RISING); // Configure the interrupt

    //attachInterrupt(digitalPinToInterrupt(pinVentilacion), pulseCounter, RISING);

    if (!sensor.begin()) {
        Serial.println("❌ Error al cargar el sensor de Distancia VL6180X !");
        while (1);
    } else {
        Serial.println("✅ VL6180X sensor de Distancia !");
        delay(300);
        //Serial.println("✅ Distancia de trabajo calculadas. ");
    }

    Serial.println("🚀 Setup completo.");
}

void loop() {
    server.handleClient();
    webSocket.loop();
    if (lecturaMaximaCMPresion <= 0) {
        /*Serial.println("Calculando distancias de trabajo del sensor.");
        lecturaMaximaCMPresion = (int)(sensor.readRange() / 100);
        Serial.print("lecturaMaximaCMPresion  ");
        Serial.println(lecturaMaximaCMPresion);
        delay(300);
        Serial.println("✅ Distancias de trabajo calculadas. ");*/
    }
    sendDatos();
    sendTiempoActual();
    mideVentilacion();
    unsigned long currentMillis = millis();
  
    if (currentMillis - lastAverageTime >= averageInterval) {
      Serial.print("currentMillis - lastAverageTime >= averageInterval  ");
      sendEstadoCargaBateria();
      if (sendingData) {
            Serial.println(currentMillis - lastAverageTime);
            lastAverageTime = currentMillis;
            sendPresiones();
            cuentaPresiones = 0;                    
        }
    }      
    delay(50);
   // Control de tiempo para batería (no bloqueante)
  if (millis() - lastBatteryCheck >= batteryInterval) {
    sendEstadoCargaBateria();
    lastBatteryCheck = millis();
  }

  delay(10); // Reducir delay general
}
