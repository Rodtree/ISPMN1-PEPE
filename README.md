# PEPE — Proyecto Experiencial Para Prácticas de Enfermería

**PEPE** (también referenciado en el código como *PEPPE*) es un maniquí robótico pensado para que estudiantes de enfermería practiquen **RCP (Reanimación Cardiopulmonar)** en condiciones realistas y con retroalimentación en tiempo real.

El robot no se limita a detectar las compresiones torácicas: también simula señales clínicas que un/a enfermero/a debe saber evaluar durante una emergencia, como la reacción pupilar a la luz y la aparición de un derrame ocular.

## ¿Qué hace?

- **Detección de RCP:** mide la presión y profundidad de las compresiones torácicas mediante un sensor de distancia (VL6180X), y la ventilación (volumen/frecuencia de aire insuflado) mediante un sensor de flujo.
- **Simulación de ojos con pantalla TFT:** los ojos del maniquí se renderizan en una pantalla, con pupilas que reaccionan a la luz que se les apunta (mediante sensores LDR), replicando el reflejo pupilar que se evalúa en una víctima real.
- **Animación de derrame ocular:** el sistema puede activar, de forma programada o aleatoria, una animación de hemorragia/derrame en el ojo, simulando un cuadro clínico adicional que el/la estudiante debe reconocer.
- **Panel del docente:** permite iniciar/detener/reiniciar la práctica, fijar la duración del ejercicio y ver en vivo un gráfico de las compresiones y ventilaciones del alumno.
- **Panel del estudiante:** muestra en tiempo real sus propias métricas durante la práctica.
- **Generación de informes:** al finalizar la práctica se pueden generar reportes con las estadísticas de la sesión (cantidad de compresiones, promedio de tiempo entre ventilaciones, volumen de aire ventilado, etc.).
- **Comunicación en tiempo real:** el ESP32 corre un servidor web + WebSocket, y transmite los datos de sensores a los paneles (docente/estudiante) sobre la red Wi-Fi propia que genera el robot (`PEPPE_001`).

## Estado del proyecto

Completo y funcional.

## Hardware

- Microcontrolador **ESP32**
- Sensor de distancia **Adafruit VL6180X** (para medir profundidad de compresión)
- Sensor de flujo (para medir ventilación)
- Pantalla **TFT** (librería TFT_eSPI) para renderizar los ojos
- Sensores **LDR** (izquierdo y derecho) para detectar cuándo se ilumina cada ojo

## Estructura del repositorio

```
PEPE/
├── platformio.ini        # Configuración de placa y dependencias (PlatformIO)
├── src/                   # Firmware del ESP32
│   ├── main.ino            # Setup, loop, servidor web y websocket
│   ├── funciones.ino        # Lógica de medición de RCP/ventilación
│   └── ojo_funciones.ino    # Lógica de ojos: pupilas, LDR y derrame ocular
├── include/               # Headers de configuración
│   ├── config.h
│   ├── ojo_config.h
│   └── serial_json_adapter.h
├── data/                  # Paneles web servidos por el ESP32 (SPIFFS)
│   ├── index.html           # Login / selección de rol
│   ├── panelDocente.html
│   ├── panelEstudiante.html
│   ├── parametros.html
│   ├── generaInforme.html
│   └── ...                  # estilos, íconos y librerías JS (chart.js, etc.)
├── app/
│   └── appRCP.aia           # Proyecto de la app mobile (MIT App Inventor)
├── assets/
│   └── logoPoli.png
├── docs/
│   └── borradores/          # HTML sueltos que no usa el firmware actualmente
└── README.md
```

## Cómo compilar y subir el firmware

Este proyecto usa [PlatformIO](https://platformio.org/) (extensión de VS Code).

1. Instalar la extensión **PlatformIO IDE** en VS Code.
2. Abrir la carpeta del proyecto.
3. Conectar la placa ESP32 por USB.
4. Compilar y subir el firmware:
   ```
   pio run --target upload
   ```
5. Subir los archivos del panel web (carpeta `data/`) al sistema de archivos del ESP32:
   ```
   pio run --target uploadfs
   ```
6. Abrir el monitor serie para ver los logs:
   ```
   pio device monitor
   ```

## Conexión

El ESP32 crea su propia red Wi-Fi:

- **SSID:** `PEPPE_001`
- **IP del servidor:** `192.168.10.1`

Conectate a esa red y accedé desde el navegador a `http://192.168.10.1` para ver el panel de login.

## Pendientes / notas

- Confirmar en `platformio.ini` el modelo exacto de placa ESP32 usado (actualmente configurado como `esp32dev` genérico).
- Si se usa una configuración custom de pines para la pantalla TFT (`TFT_eSPI`), agregar el `User_Setup.h` correspondiente o los `build_flags` en `platformio.ini`.
- La carpeta `docs/borradores/` tiene un par de HTML (`index_.html`, `head-content.html`) que no están conectados al firmware — quedaron ahí por si se recicla contenido, se pueden borrar si no hacen falta.
