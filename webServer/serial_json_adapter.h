// serial_json_adapter.h
// ESP32: puente JSON por Serial (aditivo; no rompe tu WS)
// - Agrega protocolo JSON por línea para hablar con la PC.
// - Envía métricas de RCP (profundidad/ritmo) y ventilación.
// - Acepta comandos: hello/state/command:pupils/drug (listo para futuras funciones).
//
// Uso:
//   #include "serial_json_adapter.h"
//   void setup(){ Serial.begin(115200); serial_setup(); }
//   void loop(){ serial_tick(); }
//
// Formato (UTF-8, una línea por JSON):
//   PC->ESP: {"type":"hello","role":"pc","version":1}
//            {"type":"state","payload":{...}}
//            {"type":"command","name":"scenario","value":"start|stop|..."}
//            {"type":"pupils","payload":{"mode":"dilate","value":0.8}}
//            {"type":"drug","payload":{"name":"adrenalina","dose_mg":1.0}}
//   ESP->PC: {"type":"hello","role":"esp32","version":1,"name":"PEPPE","fw":"webServer"}
//            {"type":"metrics","payload":{"cpr_depth_mm":55,"cpr_rate":110,"vent_rate":8}}
//
// Depende de ArduinoJson.
// Ajusta los nombres de variables externas si difieren en tu sketch.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// ---- Variables externas esperadas del webServer.ino ----
extern bool  sendingData;
//extern volatile int cuentaPresiones;    // se incrementa por compresión
extern int   totalVentilaciones;        // se incrementa por ventilación
//extern float lecturaMaximaCMPresion;    // baseline (cm)
extern float minRelativo, maxRelativo;  // relativos del ciclo
extern bool  alcanzoMinimo;

// ---- Estado local ----
static uint32_t _sj_lastHelloMs = 0;
static bool     _sj_handshook   = false;

// Compresiones
static int      _sj_lastCompCount = 0;
static uint32_t _sj_compTimes[16];
static uint8_t  _sj_compIdx = 0;

// Ventilaciones
static int      _sj_lastVentCount = 0;
static uint32_t _sj_ventTimes[16];
static uint8_t  _sj_ventIdx = 0;

// Profundidad (mm)
static float    _sj_lastDepthMM = 0.0f;
static String   _sj_line;

// ---- Helpers ----
static void _sj_send_json(const JsonDocument &doc) {
  String s; serializeJson(doc, s); Serial.println(s);
}

static void _sj_send_hello() {
  StaticJsonDocument<128> d;
  d["type"]    = "hello";
  d["role"]    = "esp32";
  d["version"] = 1;
  d["name"]    = "PEPPE";
  d["fw"]      = "webServer";
  _sj_send_json(d);
}

static void _sj_send_metrics() {
  // Ritmo de compresiones (cpm) con últimos intervalos
  float cpr_rate = 0.0f;
  {
    uint8_t n = 0; float sumMs = 0.0f;
    for (uint8_t i = 0; i < 8; ++i) {
      uint8_t a = (_sj_compIdx - 1 - i) & 15;
      uint8_t b = (_sj_compIdx - 2 - i) & 15;
      if (_sj_compTimes[a] && _sj_compTimes[b] && _sj_compTimes[a] > _sj_compTimes[b]) {
        sumMs += float(_sj_compTimes[a] - _sj_compTimes[b]); n++;
      }
    }
    if (n > 0 && sumMs > 0.0f) cpr_rate = 60000.0f / (sumMs / n);
  }

  // Ritmo de ventilaciones (rpm)
  float vent_rate = 0.0f;
  {
    uint8_t n = 0; float sumMs = 0.0f;
    for (uint8_t i = 0; i < 6; ++i) {
      uint8_t a = (_sj_ventIdx - 1 - i) & 15;
      uint8_t b = (_sj_ventIdx - 2 - i) & 15;
      if (_sj_ventTimes[a] && _sj_ventTimes[b] && _sj_ventTimes[a] > _sj_ventTimes[b]) {
        sumMs += float(_sj_ventTimes[a] - _sj_ventTimes[b]); n++;
      }
    }
    if (n > 0 && sumMs > 0.0f) vent_rate = 60000.0f / (sumMs / n);
  }

  StaticJsonDocument<192> d;
  d["type"] = "metrics";
  JsonObject p = d.createNestedObject("payload");
  p["cpr_rate"]     = cpr_rate;
  p["cpr_depth_mm"] = _sj_lastDepthMM;
  p["vent_rate"]    = vent_rate;
  _sj_send_json(d);
}

static void _sj_update_depth_from_cycle() {
  // Estima profundidad: (lecturaMaximaCMPresion - minRelativo) [cm] → mm
  float depth_cm = lecturaMaximaCMPresion - minRelativo;
  if (depth_cm < 0)  depth_cm = 0;
  if (depth_cm > 10) depth_cm = 10; // límite sanidad
  _sj_lastDepthMM = depth_cm * 10.0f;
}

// ---- API pública ----
static void serial_setup() {
  _sj_lastHelloMs = millis();
  _sj_send_hello();
}

static void _sj_handle_msg(const String &line) {
  StaticJsonDocument<256> d;
  if (deserializeJson(d, line)) return;
  const char* type = d["type"]; if (!type) return;

  if (!strcmp(type, "hello") && !strcmp(d["role"] | "", "pc")) {
    _sj_handshook = true; return;
  }
  if (!strcmp(type, "state")) {
    // opcional: espejo del estado PC
    return;
  }
  if (!strcmp(type, "command")) {
    const char* name  = d["name"]  | "";
    const char* value = d["value"] | "";
    if (!strcmp(name, "scenario")) {
      if (!strcmp(value, "start"))      sendingData = true;
      else if (!strcmp(value, "stop"))  sendingData = false;
      else                               sendingData = !sendingData; // toggle
    }
    return;
  }
  if (!strcmp(type, "pupils")) {
    // TODO: integrar con controlador de ojos (dilate/anisocoria/seizure)
    return;
  }
  if (!strcmp(type, "drug")) {
    // TODO: log/reacción local opcional
    return;
  }
}

static void serial_tick() {
  const uint32_t now = millis();

  // Re-hello si no hay handshake
  if (!_sj_handshook && (now - _sj_lastHelloMs) > 1500) {
    _sj_send_hello(); _sj_lastHelloMs = now;
  }

  // Leer JSON por línea
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      String line = _sj_line; _sj_line = "";
      line.trim(); if (line.length()) _sj_handle_msg(line);
    } else if (c != '\r') {
      _sj_line += c;
      if (_sj_line.length() > 512) _sj_line.remove(0, 128);
    }
  }

  // Detectar eventos de compresión
  if (cuentaPresiones != _sj_lastCompCount) {
    _sj_lastCompCount = cuentaPresiones;
    _sj_compTimes[_sj_compIdx++ & 15] = now;
  }

  // Detectar eventos de ventilación
  if (totalVentilaciones != _sj_lastVentCount) {
    _sj_lastVentCount = totalVentilaciones;
    _sj_ventTimes[_sj_ventIdx++ & 15] = now;
  }

  // Estimar profundidad periódicamente mientras se envían datos
  static uint32_t lastDepthCheck = 0;
  if (sendingData && (now - lastDepthCheck) > 100) {
    lastDepthCheck = now;
    _sj_update_depth_from_cycle();
  }

  // Enviar métricas a 5 Hz
  static uint32_t lastMetrics = 0;
  if ((now - lastMetrics) > 200) {
    lastMetrics = now;
    _sj_send_metrics();
  }
}
