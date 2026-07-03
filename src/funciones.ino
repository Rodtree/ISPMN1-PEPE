#ifndef FUNCIONES_H
#define FUNCIONES_H

//#include "funciones.h"
//#include "config.h"

/*-----------------------------*/
void mideVentilacion() {
    static unsigned long lastPrintTime = 0;  // Para evitar imprimir demasiado rápido

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
        totalVentilaciones += 1;  // Contar un evento de ventilación
        totalDuracionVentilaciones += sumaDuracion;
        totalAireVentilado += sumaVentilacion;

        // Calcular el tiempo promedio entre ventilaciones
        if (totalVentilaciones > 1) {
            totalTiempoEntreVentilaciones += (lastEndTime - lastFlowTime);  // Tiempo entre el inicio y el fin del flujo
        }

        // Reiniciar arrays e índice
        indiceVentilacion = 0;
        sumaDuracion = 0;
        sumaVentilacion = 0;
    }
}

/*-----------------------------*/
void sendVentilacion() {
    DynamicJsonDocument doc(1024);
    doc["type"] = "ventilacion";
    doc["cm3ventilados"] = sumaVentilacion;
    doc["duracionVentilacion"] = sumaDuracion/1000;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
    Serial.println("Datos de ventilación enviados.");  // Depuración
}

void sendEstadisticasVentilacion() {
    DynamicJsonDocument doc(1024);
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
        doc["duracionPromedioVentilaciones"] = (float)(totalDuracionVentilaciones / totalVentilaciones)/1000000;
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

void sendPresiones() {
    DynamicJsonDocument doc(512);
    Serial.print("Enviando la cuenta de presiones  ");
    Serial.println(cuentaPresiones);
    doc["type"] = "presiones";
    doc["cuentaPress"] = cuentaPresiones;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    float dataIn = (float) atof((const char *) &payload[0]);
  }
  else if (type == WStype_CONNECTED) {
    //Client connection detected
    Serial.print("Client: ");
    Serial.print(num);
    Serial.println(" Connected");
  }
  else if (type == WStype_DISCONNECTED) {
    //Client disconnection detected
    Serial.print("Client: ");
    Serial.print(num);
    Serial.println(" Disconnected");
   }
}
void handleWebSocketMessage(uint8_t *data, size_t len) {
    String rawMessage = String((char *)data).substring(0, len);
    DynamicJsonDocument doc(512);
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
        //doc["lecturaMaximaCMPresion"] = lecturaMaximaCMPresion;
        /*--- Validar Accion desde el panel docente ---*/

        if (estado == "start") {
            Serial.print("pasando por estado === start  ");
            globalEstudiante = doc["estudiante"].as<String>();
            globalDuracionPrueba = doc["duracionPrueba"].as<int>();
            sendingData = true;
            accionEnvio= "start";
            //busco distancia maxima de lectura
            //sensor.setOffset(120);
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
            lastStatsTime = millis(); // Reiniciar el temporizador de estadísticas
        } else if (estado == "stop") {
          Serial.print("pasando por estado === stop  ");
            sendingData = false;
            cuentaPresiones = 0;
            sumaDuracion = 0;
            sumaVentilacion = 0;
            sendEstadisticasVentilacion();
            /*----- enviar datos a Estudiante y profe ---*/
            
            DynamicJsonDocument stopDoc(128);
            stopDoc["type"] = "datosMediciones";
            stopDoc["conexion"] = "stop";
            String jsonString;
            serializeJson(stopDoc, jsonString);
            webSocket.broadcastTXT(jsonString);
            accionEnvio= "stop";
            //sendDatos();
            iniciaGrafica();
             // Enviar estadísticas al detener
        } else if (estado == "reset") {
          Serial.print("pasando por estado === reset  ");
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
            DynamicJsonDocument resetDoc(128);
            resetDoc["type"] = "datosMediciones";
            resetDoc["conexion"] = "reset";
            accionEnvio= "reset";
            String jsonString;
            serializeJson(resetDoc, jsonString);
            webSocket.broadcastTXT(jsonString);
        }
        sendDatosIniciales();
    }
    /*-----------  validar acceso y cantidad de usuarios----*/
    /*-----------  solo valido para el index ---------------*/
        if (doc["type"] == "validarContrasena") {
            String contrasena = doc["contrasena"].as<String>();
            bool contrasenaValida = (contrasena == "123456789"); // Validar la contraseña
    
            DynamicJsonDocument respuesta(128);
            respuesta["type"] = "respuestaContrasena";
            respuesta["valida"] = contrasenaValida;
            respuesta["conexionesActivas"] = conexionesActivas;
            String jsonString;
            serializeJson(respuesta, jsonString);
            webSocket.broadcastTXT(jsonString); // Enviar respuesta al cliente
        }
}

void sendDatosIniciales() {
    /*-------------- esto es para panel Estudiante -------*/
    DynamicJsonDocument doc(512);
    doc["type"] = "datosIniciales";
    doc["estudiante"] = globalEstudiante;    
    doc["estadoConexion"] = sendingData;
    //doc["accionEnvio"] = accionEnvio;
    doc["lecturaMaximaCMPresion"] =lecturaMaximaCMPresion; // Máximo 
    doc["duracionPrueba"] = globalDuracionPrueba;
    //doc["lecturaMaximaCMPresion"] = lecturaMaximaCMPresion;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}
void iniciaGrafica(){ 
    /*-------------- esto es para panel Docente-------*/
    DynamicJsonDocument doc(128);
    doc["type"] = "iniciaGrafica";
    doc["lecturaMaximaCMPresion"] = lecturaMaximaCMPresion;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);   
}

// Variables para almacenar los valores máximos y mínimos relativos

void sendDatos() {
    if (sendingData) {
        float cmPresion = sensor.readRange() / 10.0; // Distancia en cm
        // Calcular la diferencia respecto al máximo relativo
        float diferencia = maxRelativo - cmPresion;

        // Detectar si se está comprimiendo (diferencia > 1 cm)
        if (diferencia > 1.0) {
            if (!comprimiendo) {
                // Comienza una compresión
                comprimiendo = true;
                minRelativo = cmPresion; // Inicializar el mínimo relativo
            } else {
                // Actualizar el mínimo relativo durante la compresión
                if (cmPresion < minRelativo) {
                    minRelativo = cmPresion;
                }
            }
        } else {
            // Detectar si se está liberando la compresión (diferencia <= 1 cm)
            if (comprimiendo) {
                // Comienza la liberación
                comprimiendo = false;
                maxRelativo = cmPresion; // Inicializar el máximo relativo
            } else {
                // Actualizar el máximo relativo durante la liberación
                if (cmPresion > maxRelativo) {
                    maxRelativo = cmPresion;
                }
            }
        }

        // Verificar si se alcanzó un mínimo relativo
        if (!alcanzoMinimo && cmPresion == minRelativo) {
            alcanzoMinimo = true;
            cuentaPresiones++;
            //sendPresiones();
        }

        // Verificar si se volvió al máximo inicial
        if (alcanzoMinimo && cmPresion >= lecturaMaximaCMPresion) {
            // Calcular la diferencia entre el máximo y el mínimo relativo
            float diferenciaMaxMin = maxRelativo - minRelativo;
            // Decidir qué valor enviar (máximo o mínimo)
            float valorAEnviar = (diferenciaMaxMin > 2.0) ? minRelativo : maxRelativo;
            
            // Enviar el valor seleccionado
            DynamicJsonDocument doc(1024);
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
}
/*------------------*/
void sendTiempoActual() {
    if (sendingData) {
        DynamicJsonDocument doc(128);
        doc["type"] = "tiempoActual";
        doc["tiempo"] = millis();
        String jsonString;
        serializeJson(doc, jsonString);
        webSocket.broadcastTXT(jsonString);
    }
}

/*------------------*/
// NOTA: esta función no existía en el repo original (se la llamaba desde
// loop() en main.ino pero nunca se había escrito). La arme usando las
// constantes de divisor de voltaje (R1, R2, Vmin, Vmax, VREF, alpha) que
// ya estaban declaradas y calibradas en main.ino, pero está sin probar en
// hardware real -- confirmá que el porcentaje resultante tenga sentido con
// tu batería antes de confiar en el dato.
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

    DynamicJsonDocument doc(128);
    doc["type"] = "estadoBateria";
    doc["voltaje"] = batteryVoltage;
    doc["porcentaje"] = porcentaje;
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);

    Serial.print("Batería: ");
    Serial.print(batteryVoltage);
    Serial.print("V (");
    Serial.print(porcentaje);
    Serial.println("%)");
}

#endif
