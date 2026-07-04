// ventilacion.ino
// Medición de ventilación a partir del sensor de flujo (interrupción en
// pinVentilacion) y envío de estadísticas por WebSocket.

void mideVentilacion() {
    if (pulseCount > 0) {  // Si hay flujo detectado
        detachInterrupt(digitalPinToInterrupt(pinVentilacion));  // Deshabilitar interrupción durante el cálculo

        flowDuration = millis() - lastFlowTime;  // Calcular duración del flujo
        frequency = (float)pulseCount / (flowDuration / 1000.0);  // Frecuencia en Hz
        flowRateL_min = frequency / calibrationFactor;  // Cálculo del caudal en L/min
        flowRateL_Beath = (flowRateL_min / 60) * flowDuration;  // cm³ por respiración

        // Almacenar datos en arrays
        if (indiceVentilacion < MAX_REGISTROS) {
            duraciones[indiceVentilacion] = flowDuration;
            ventilaciones[indiceVentilacion] = flowRateL_Beath;
            indiceVentilacion++;
            pulseCount = 0;  // Reiniciar el conteo de pulsos
            lastEndTime = millis();  // Guardar el tiempo de finalización del flujo
        }

        attachInterrupt(digitalPinToInterrupt(pinVentilacion), pulseCounter, RISING);  // Rehabilitar interrupción
    }

    // Si no hay flujo durante más de 2 segundos y hay datos almacenados
    if (pulseCount == 0 && millis() - lastEndTime > 2000 && indiceVentilacion > 0) {
        Serial.println("\n====================");
        Serial.println("Resumen de Ventilaciones:");
        Serial.print("indiceVentilacion ");
        Serial.println(indiceVentilacion);

        for (int i = 0; i < indiceVentilacion; i++) {
            sumaDuracion += duraciones[i];
            sumaVentilacion += ventilaciones[i];
        }
        Serial.print("Tiempo Total de ventilación: ");
        Serial.print(sumaDuracion);
        Serial.println(" ms");

        Serial.print("Volumen de ventilación: ");
        Serial.print(sumaVentilacion);
        Serial.println(" cm³");
        Serial.println("====================\n");

        if (sumaVentilacion > 0) {
            sendVentilacion();
        }

        // Actualizar estadísticas de ventilación
        totalVentilaciones += 1;
        totalDuracionVentilaciones += sumaDuracion;
        totalAireVentilado += sumaVentilacion;

        // Calcular el tiempo promedio entre ventilaciones
        if (totalVentilaciones > 1) {
            totalTiempoEntreVentilaciones += (lastEndTime - lastFlowTime);
        }

        // Reiniciar arrays e índice
        indiceVentilacion = 0;
        sumaDuracion = 0;
        sumaVentilacion = 0;
    }
}

void sendVentilacion() {
    JsonDocument doc;
    doc["type"] = "ventilacion";
    doc["cm3ventilados"] = sumaVentilacion;
    doc["duracionVentilacion"] = sumaDuracion / 1000;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
    Serial.println("Datos de ventilación enviados.");
}

void sendEstadisticasVentilacion() {
    JsonDocument doc;
    doc["type"] = "estadisticasVentilacion";
    doc["totalVentilaciones"] = totalVentilaciones;

    // Calcular el tiempo promedio entre ventilaciones
    if (totalVentilaciones > 1) {
        doc["tiempoPromedioEntreVentilaciones"] = (float)totalTiempoEntreVentilaciones / (totalVentilaciones - 1);
    } else {
        doc["tiempoPromedioEntreVentilaciones"] = 0;
    }

    // Calcular la duración promedio de las ventilaciones
    if (totalVentilaciones > 0) {
        doc["duracionPromedioVentilaciones"] = (float)(totalDuracionVentilaciones / totalVentilaciones) / 1000000;
    } else {
        doc["duracionPromedioVentilaciones"] = 0;
    }

    // Calcular el volumen promedio de aire ventilado
    if (totalVentilaciones > 0) {
        doc["airePromedioVentilado"] = (float)totalAireVentilado / totalVentilaciones;
    } else {
        doc["airePromedioVentilado"] = 0;
    }

    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
    Serial.println("Enviando estadísticas de ventilación:");
    Serial.println(jsonString);
}
