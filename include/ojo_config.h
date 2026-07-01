#ifndef OJO_CONFIG_H
#define OJO_CONFIG_H

//#define SYMMETRICAL_EYELID

// Enable ONE of these #includes -- HUGE graphics tables for various eyes:
#include "defaultEye.h"           // Standard human-ish hazel eye -OR-
#pragma once
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
// Configuración CORREGIDA de pines:
#define TFT_COUNT 2
#define TFT1_CS 13     // de frente  Display derecho (CS) 
#define TFT2_CS 4    // de frente  Display izquierdo (CS) - Cambiado a pin 15
#define TFT_DC 2      // Pin DC compartido
#define TFT_RST -1    // Deshabilitar reset (o usar pin separado)
#define TFT_MOSI 23   // MOSI (SDA)
#define TFT_SCLK 18   // SCK (SCL)
#define TFT_MISO -1   // No necesario
//#define TFT_MOSI -1//23   // Pines SPI
//#define TFT_SCLK -1//18
//#define TFT_MISO -1   // No necesario para displays GC9A01
//#define TFT_WIDTH  240   // Ancho en píxeles
//#define TFT_HEIGHT 240   // Alto en píxeles (GC9D01 es circular, pero la resolución es 240x240)
//#define SPI_FREQUENCY  20000000  // GC9A01 soporta hasta 40MHz

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

// Configuración de iris
#define IRIS_SMOOTH
#define IRIS_MIN 10    // 20/64 = 31% del tamaño máximo
#define IRIS_MAX 90    // 64/64 = 100% del tamaño máximo

// Otras configuraciones
//#define TRACKING
#define AUTOBLINK

//  #define LIGHT_PIN      -1 // Light sensor pin
  #define LIGHT_CURVE  0.33 // Light sensor adjustment curve
  #define LIGHT_MIN       50 // Minimum useful reading from light sensor
  #define LIGHT_MAX    2000 // Maximum useful reading from sensor

#define IRIS_SMOOTH         // If enabled, filter input from IRIS_PIN
#if !defined(IRIS_MIN)      // Each eye might have its own MIN/MAX
  #define IRIS_MIN       10 // Iris size (0-1023) in brightest light
#endif
#if !defined(IRIS_MAX)
  #define IRIS_MAX      90 // Iris size (0-1023) in darkest light
#endif


#endif
