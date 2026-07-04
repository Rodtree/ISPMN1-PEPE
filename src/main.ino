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
TFT_eSPI tft;           // A single instance is used for 1 or 2 displays
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
String accionEnvio = "";

/*----------- variables control de presiones -----*/
unsigned long lastAverageTime = 0;
const unsigned long averageInterval = 29000; // envío cada 30s de la cuenta de presiones
int cuentaPresiones = 0;
// Variables para almacenar los valores máximos y mínimos relativos
float maxRelativo = 0; // Máximo relativo inicial (distancia máxima)
float minRelativo = 0; // Mínimo relativo inicial (distancia mínima)
bool comprimiendo = false; // Indica si se está realizando una compresión
bool alcanzoMinimo = false; // Indica si se alcanzó un mínimo relativo

/*---------------------*/
unsigned long ritmoVentilacion = 0;

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
float calibrationFactor = 7.5;  // Factor de conversión del sensor de ventilación

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
// solo para el envío de estado de la batería al inicio
bool enviarBateria = true;

#include "serial_json_adapter.h"

void pulseCounter() {
    pulseCount++;  // Incrementar el contador de pulsos
}

/*-------------------------------------*/
// === Configuración de batería ===
const float R1 = 22000.0;           // 22kΩ (entre batería y pin)
const float R2 = 12000.0;           // 12kΩ (entre pin y GND)
const float Vmin = 3.0;             // Voltaje mínimo (0%)
const float Vmax = 4.2;             // Voltaje máximo (100%)
const float VREF = 3.26;            // Valor medido entre 3v3 y gnd
const float alpha = 0.3;            // Factor de suavizado (0.1 = más liso, 0.9 = más rápido)
float filteredADC = 0;              // Variable para el filtro pasa-bajos
bool filtroInicializado = false;
unsigned long lastBatteryCheck = 0; // Control de tiempo
const int batteryInterval = 60000;  // Muestreo cada 60 segundos

/*-------------------------------------*/
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Iniciando ESP32...");

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
    // Antes: Serial.println("... " + WiFi.softAPIP().toString());
    // Concatenar String con '+' crea objetos temporales en heap en cada
    // llamada; con print() separado se evita esa fragmentación.
    Serial.print("✅ AP iniciado. IP: ");
    Serial.println(WiFi.softAPIP());

    registerHttpRoutes(); // definido en web_routes.ino

    // Calibración inicial del filtro de batería (promedio de 50 lecturas)
    float sumaInicial = 0;
    for (int i = 0; i < 50; i++) {
        sumaInicial += analogRead(pinBateria);
        delay(10);
    }
    filteredADC = sumaInicial / 50; // Inicializar con promedio

    server.begin();
    Serial.println("✅ Servidor web iniciado en el puerto 80");

    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent); // definido en websocket_handler.ino
    Serial.println("✅ WebSockets iniciado en el puerto 81");

    pinMode(pinVentilacion, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pinVentilacion), pulseCounter, RISING);

    if (!sensor.begin()) {
        Serial.println("❌ Error al cargar el sensor de Distancia VL6180X !");
        while (1);
    } else {
        Serial.println("✅ VL6180X sensor de Distancia !");
        delay(300);
    }

    // --- Sistema de ojos (pantallas TFT + reacción a la luz + derrame) ---
    tft.init();
    initEyes();
    Serial.println("✅ Ojos inicializados.");

    Serial.println("🚀 Setup completo.");
}

void loop() {
    server.handleClient();
    webSocket.loop();

    sendDatos();
    sendTiempoActual();
    mideVentilacion();
    actualizarOjos();

    unsigned long currentMillis = millis();
    actualizarPresiones(currentMillis);
    actualizarBateria(currentMillis);

    // Antes había 60ms de delay() bloqueante fijo (delay(50) + delay(10))
    // sin ninguna razón documentada. Se reemplaza por un delay(1) mínimo,
    // que es la práctica recomendada en ESP32 para dejarle aire al
    // scheduler/WiFi sin frenar la respuesta del WebSocket ni la animación.
    delay(1);
}
