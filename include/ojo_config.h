#ifndef OJO_CONFIG_H
#define OJO_CONFIG_H

//#define SYMMETRICAL_EYELID

// Enable ONE of these #includes -- HUGE graphics tables for various eyes:
#include "defaultEye.h"           // Standard human-ish hazel eye -OR-
//AZUL SCL 23
//VERDE SCA 21
//AMARILLO DC  2
//cs blanco
//dc amarillo
//sda verde
//scl azul

#define LDRojoizquierdo 26
#define LDRojoderecho 33
#define INITIAL_PUPIL_SIZE 512  // Valor medio entre IRIS_MIN y IRIS_MAX
// ====== Configuración de velocidad pupilar ======
#define PUPIL_UPDATE_BASE_MS 250    // Tiempo base de actualización
#define PUPIL_LEFT_SPEED_FACTOR 0.8 // Ojo izquierdo 20% más rápido
#define PUPIL_RIGHT_SPEED_FACTOR 0.8 // Ojo derecho 20% 
#define PUPIL_LEFT_VARIATION 15     // ±15% variación ojo izquierdo
#define PUPIL_RIGHT_VARIATION 15    // ±30% variación ojo derecho

// Configuración de displays GC9A01
// NOTA: TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO, TFT_CS y el driver
// (GC9A01_DRIVER) ahora se definen en build_flags de platformio.ini -- antes
// estaban acá Y en el User_Setup.h por defecto de la librería TFT_eSPI al
// mismo tiempo, lo que generaba warnings de "redefined" y, peor, dejaba
// activo el driver por defecto de la librería en vez de GC9A01. Acá solo
// quedan los pines de CS por-ojo, que son propios de este proyecto (no
// son macros que use TFT_eSPI internamente).
#define TFT_COUNT 2
#define TFT1_CS 13     // de frente  Display derecho (CS)
#define TFT2_CS 4      // de frente  Display izquierdo (CS) - Cambiado a pin 15

// Rotaciones (prueba con 0, 1, 2 o 3 si no se ve correctamente)
#define TFT_1_ROT 0   // Rotación display izquierdo
#define TFT_2_ROT 0   // Rotación display derecho

// Posiciones de los ojos
#define EYE_1_XPOSITION 50    // Ajustar según necesidad
#define EYE_2_XPOSITION 100    // Ajustar según necesidad

// Configuración de ojos
#define NUM_EYES 2
#define BLINK_PIN -1
#define LH_WINK_PIN -1
#define RH_WINK_PIN -1

// Estructura de información del ojo
typedef struct {
  int8_t select;    // Pin CS
  int8_t wink;      // Pin wink (-1 si no hay)
  uint8_t rotation; // Rotación
  int16_t xposition;// Posición X
} eyeInfo_t;

eyeInfo_t eyeInfo[] = {
  {TFT1_CS, LH_WINK_PIN, TFT_1_ROT, EYE_1_XPOSITION}, // Ojo izquierdo
  {TFT2_CS, RH_WINK_PIN, TFT_2_ROT, EYE_2_XPOSITION}  // Ojo derecho
};

// ====== Buffer de renderizado y máquina de estados de parpadeo ======
// NOTA: este bloque no estaba en el repo original. Falta en el código fuente
// (el propio comentario en ojo_funciones.ino decía "definido en config.h" pero
// nunca se llegó a escribir). Lo reconstruí a partir del ejemplo oficial
// "Animated_Eyes" que trae la librería TFT_eSPI, que es la base de la que se
// adaptó este código, ajustado a los pines/NUM_EYES ya definidos arriba.
#define BUFFER_SIZE 1024 // 128 a 1024 es lo recomendado por TFT_eSPI
#ifdef USE_DMA
  #define BUFFERS 2 // 2 buffers alternados con DMA
#else
  #define BUFFERS 1 // 1 buffer sin DMA
#endif
uint16_t pbuffer[BUFFERS][BUFFER_SIZE]; // Buffer de renderizado de píxeles
bool dmaBuf = 0;                        // Selección de buffer DMA

#define NOBLINK 0 // No está parpadeando
#define ENBLINK 1 // Párpado cerrándose
#define DEBLINK 2 // Párpado abriéndose

typedef struct {
  uint8_t  state;      // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duración del estado actual (micros)
  uint32_t startTime;   // Momento (micros) del último cambio de estado
} eyeBlink;

struct { // Estructura por ojo
  int16_t  tft_cs;    // Pin de chip select del display de este ojo
  eyeBlink blink;     // Estado actual de parpadeo
  int16_t  xposition; // Posición X de la imagen del ojo
} eye[NUM_EYES];

uint32_t startTime; // Usado para el cálculo de FPS en el loop de animación

// Configuración de iris
// NOTA: antes este bloque definía IRIS_SMOOTH/IRIS_MIN/IRIS_MAX y después
// los "redefinía" bajo un #if !defined(...) que nunca se disparaba (ya
// estaban definidos arriba, era código muerto). Se dejan una sola vez.
#define IRIS_SMOOTH         // Si está activo, filtra la entrada de IRIS_PIN
#define IRIS_MIN 10    // 20/64 = 31% del tamaño máximo
#define IRIS_MAX 90    // 64/64 = 100% del tamaño máximo

// Otras configuraciones
//#define TRACKING
#define AUTOBLINK

//  #define LIGHT_PIN      -1 // Light sensor pin
  #define LIGHT_CURVE  0.33 // Light sensor adjustment curve
  #define LIGHT_MIN       50 // Minimum useful reading from light sensor
  #define LIGHT_MAX    2000 // Maximum useful reading from sensor

#endif
