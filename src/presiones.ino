// presiones.ino
// Lógica de detección de compresiones (RCP) a partir del sensor de distancia,
// y envío periódico de esa cuenta + el timer de la práctica en curso.

void sendDatos() {
    if (!sendingData) return;

    float cmPresion = sensor.readRange() / 10.0; // Distancia en cm
    // Calcular la diferencia respecto al máximo relativo
    float diferencia = maxRelativo - cmPresion;

    // Detectar si se está comprimiendo (diferencia > 1 cm)
    if (diferencia > 1.0) {
        if (!comprimiendo) {
            // Comienza una compresión
            comprimiendo = true;
            minRelativo = cmPresion; // Inicializar el mínimo relativo
        } else if (cmPresion < minRelativo) {
            // Actualizar el mínimo relativo durante la compresión
            minRelativo = cmPresion;
        }
    } else {
        // Detectar si se está liberando la compresión (diferencia <= 1 cm)
        if (comprimiendo) {
            // Comienza la liberación
            comprimiendo = false;
            maxRelativo = cmPresion; // Inicializar el máximo relativo
        } else if (cmPresion > maxRelativo) {
            // Actualizar el máximo relativo durante la liberación
            maxRelativo = cmPresion;
        }
    }

    // Verificar si se alcanzó un mínimo relativo
    if (!alcanzoMinimo && cmPresion == minRelativo) {
        alcanzoMinimo = true;
        cuentaPresiones++;
    }

    // Verificar si se volvió al máximo inicial
    if (alcanzoMinimo && cmPresion >= lecturaMaximaCMPresion) {
        // Calcular la diferencia entre el máximo y el mínimo relativo
        float diferenciaMaxMin = maxRelativo - minRelativo;
        // Decidir qué valor enviar (máximo o mínimo)
        float valorAEnviar = (diferenciaMaxMin > 2.0) ? minRelativo : maxRelativo;

        JsonDocument doc;
        doc["type"] = "datosMediciones";
        doc["conexion"] = sendingData;
        doc["accionEnvio"] = accionEnvio;
        doc["cmPresion"] = valorAEnviar; // Enviar el valor seleccionado
        String jsonString;
        serializeJson(doc, jsonString);
        webSocket.broadcastTXT(jsonString);

        // Reiniciar el estado para la próxima compresión
        alcanzoMinimo = false;
        maxRelativo = lecturaMaximaCMPresion;
        minRelativo = lecturaMaximaCMPresion;
    }
}

/*------------------*/
// Antes se llamaba en cada vuelta de loop() sin límite -> mandaba un JSON
// por WebSocket potencialmente miles de veces por segundo, puro tráfico
// innecesario para un dato que solo alimenta un timer visual. Se throttlea
// a 5 Hz (200ms), de sobra para que se vea fluido en el panel.
void sendTiempoActual() {
    if (!sendingData) return;

    static uint32_t lastTiempoSend = 0;
    uint32_t now = millis();
    if (now - lastTiempoSend < 200) return;
    lastTiempoSend = now;

    JsonDocument doc;
    doc["type"] = "tiempoActual";
    doc["tiempo"] = now;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}

void sendPresiones() {
    JsonDocument doc;
    Serial.print("Enviando la cuenta de presiones  ");
    Serial.println(cuentaPresiones);
    doc["type"] = "presiones";
    doc["cuentaPress"] = cuentaPresiones;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}

int calculoModa() {
    int mode = -1;
    int maxCount = 0;
    for (const auto& pair : valueFrequency) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            mode = pair.first;
        }
    }
    return mode;
}

// Cada 30 segundos (mientras hay una práctica en curso), reporta y reinicia
// el contador de compresiones. Existe solo para que loop() en main.ino no
// tenga que conocer este temporizador no bloqueante (patrón millis()).
void actualizarPresiones(unsigned long currentMillis) {
    if (currentMillis - lastAverageTime < averageInterval) return;

    lastAverageTime = currentMillis;
    if (sendingData) {
        sendPresiones();
        cuentaPresiones = 0;
    }
}
