// bateria.ino
// Lectura del divisor resistivo de batería, filtrado y reporte por WebSocket.

void sendEstadoCargaBateria() {
    // Filtro pasa-bajos sobre la lectura cruda del ADC
    int rawADC = analogRead(pinBateria);
    filteredADC = alpha * rawADC + (1.0 - alpha) * filteredADC;

    // ADC (12 bits en ESP32) -> voltaje en el pin -> voltaje real de batería
    // (se revierte el divisor resistivo R1/R2)
    float pinVoltage = (filteredADC / 4095.0) * VREF;
    float batteryVoltage = pinVoltage * (R1 + R2) / R2;

    // Voltaje -> porcentaje, acotado entre 0 y 100
    float porcentaje = (batteryVoltage - Vmin) / (Vmax - Vmin) * 100.0;
    if (porcentaje > 100.0) porcentaje = 100.0;
    if (porcentaje < 0.0) porcentaje = 0.0;

    // type/campo deben coincidir EXACTO con lo que espera panelDocente.html:
    // if (data.type === "cargaBateria") ... data.porcentajeCargaBatt
    JsonDocument doc;
    doc["type"] = "cargaBateria";
    doc["porcentajeCargaBatt"] = porcentaje;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);

    Serial.print("Batería: ");
    Serial.print(batteryVoltage);
    Serial.print("V (");
    Serial.print(porcentaje);
    Serial.println("%)");
}

// Cada `batteryInterval` ms, mide y reporta la carga de la batería. Existe
// solo para que loop() en main.ino no tenga que conocer este temporizador.
void actualizarBateria(unsigned long currentMillis) {
    if (currentMillis - lastBatteryCheck < batteryInterval) return;

    lastBatteryCheck = currentMillis;
    sendEstadoCargaBateria();
}
