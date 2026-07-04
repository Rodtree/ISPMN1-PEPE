// websocket_handler.ino
// Manejo de conexiones y mensajes entrantes por WebSocket.
//
// NOTA: existía además una función webSocketEvent() en el viejo funciones.ino
// que nunca se registraba como callback (código muerto) -- se eliminó. El
// callback real es este onWebSocketEvent(), que antes vivía como lambda
// inline dentro de setup() en main.ino.

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            if (conexionesActivas < 3) {
                conexionesActivas++;
                Serial.printf("🟢 Cliente %u conectado. Conexiones activas: %d\n", num, conexionesActivas);
            } else {
                // Rechazar la conexión si ya hay 3 conexiones activas
                webSocket.disconnect(num);
                Serial.printf("🔴 Conexión rechazada. Límite de 3 conexiones alcanzado.\n");
            }
            break;

        case WStype_DISCONNECTED:
            if (conexionesActivas > 0) {
                conexionesActivas--;
            }
            Serial.printf("🔴 Cliente %u desconectado. Conexiones activas: %d\n", num, conexionesActivas);
            break;

        case WStype_TEXT:
            Serial.printf("📩 Mensaje recibido: %s\n", payload);
            handleWebSocketMessage(payload, length);
            break;

        case WStype_ERROR:
            // Antes: Serial.printf("WStype_ERROR", payload) -- el formato no
            // tenía "%s" pero se le pasaba payload como argumento (comportamiento
            // indefinido / warning del compilador). No tiene sentido además
            // tratar de parsear un payload de un evento de error como si
            // fuera un mensaje JSON válido.
            Serial.println("⚠️ WStype_ERROR");
            break;

        default:
            break;
    }
}

void handleWebSocketMessage(uint8_t *data, size_t len) {
    String rawMessage = String((char *)data).substring(0, len);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, rawMessage);
    if (error) {
        Serial.print("Error al parsear JSON: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc["type"] == "envioComandoaESP") {
        String estado = doc["estadoConexion"];
        Serial.print("estadoConexion  ");
        Serial.println(estado);

        /*--- Validar Acción desde el panel docente ---*/
        if (estado == "start") {
            globalEstudiante = doc["estudiante"].as<String>();
            globalDuracionPrueba = doc["duracionPrueba"].as<int>();
            sendingData = true;
            accionEnvio = "start";

            // busco distancia máxima de lectura
            lecturaMaximaCMPresion = (int)(sensor.readRange() / 10);
            maxRelativo = lecturaMaximaCMPresion; // Máximo relativo inicial
            minRelativo = lecturaMaximaCMPresion; // Mínimo relativo inicial
            Serial.print("lecturaMaximaCMPresion  ");
            Serial.println(lecturaMaximaCMPresion);

            // Reiniciar estadísticas al iniciar
            totalVentilaciones = 0;
            totalTiempoEntreVentilaciones = 0;
            totalDuracionVentilaciones = 0;
            totalAireVentilado = 0.0;
            lastStatsTime = millis();
        } else if (estado == "stop") {
            sendingData = false;
            cuentaPresiones = 0;
            sumaDuracion = 0;
            sumaVentilacion = 0;
            sendEstadisticasVentilacion();

            /*----- enviar datos a Estudiante y profe ---*/
            JsonDocument stopDoc;
            stopDoc["type"] = "datosMediciones";
            stopDoc["conexion"] = "stop";
            String jsonString;
            serializeJson(stopDoc, jsonString);
            webSocket.broadcastTXT(jsonString);
            accionEnvio = "stop";
            iniciaGrafica();
        } else if (estado == "reset") {
            sendingData = false;
            cuentaPresiones = 0;
            sumaDuracion = 0;
            sumaVentilacion = 0;
            totalVentilaciones = 0;
            totalTiempoEntreVentilaciones = 0;
            totalDuracionVentilaciones = 0;
            totalAireVentilado = 0.0;
            maxRelativo = lecturaMaximaCMPresion;
            minRelativo = lecturaMaximaCMPresion;
            comprimiendo = false;

            /*----- enviar datos a Estudiante y profe ---*/
            JsonDocument resetDoc;
            resetDoc["type"] = "datosMediciones";
            resetDoc["conexion"] = "reset";
            accionEnvio = "reset";
            String jsonString;
            serializeJson(resetDoc, jsonString);
            webSocket.broadcastTXT(jsonString);
        }
        sendDatosIniciales();
    }
    // NOTA: la validación de contraseña del docente se sacó de acá y pasó
    // a ser una constante en el JS de index.html -- no hacía falta que
    // viajara por WebSocket, no es un dato que necesite protección real.
}

void sendDatosIniciales() {
    /*-------------- esto es para panel Estudiante -------*/
    JsonDocument doc;
    doc["type"] = "datosIniciales";
    doc["estudiante"] = globalEstudiante;
    doc["estadoConexion"] = sendingData;
    doc["lecturaMaximaCMPresion"] = lecturaMaximaCMPresion;
    doc["duracionPrueba"] = globalDuracionPrueba;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}

void iniciaGrafica() {
    /*-------------- esto es para panel Docente -------*/
    JsonDocument doc;
    doc["type"] = "iniciaGrafica";
    doc["lecturaMaximaCMPresion"] = lecturaMaximaCMPresion;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}
