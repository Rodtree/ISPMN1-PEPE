// ====== Colores personalizados ======
#define COLOR_PIEL   0xFD76   // tono piel claro
#define COLOR_BLANCO TFT_WHITE
#define COLOR_NEGRO  TFT_BLACK
#define COLOR_PARPA  0xFD76   // mismo tono piel
#define COLOR_IRIS   0x04BA  
#define COLOR_IRIS_DARK 0x0380 // versión más oscura para variación
#define LDRizquierdo 26
#define LDRDerecho 33
// ====== Variables para control de pupila por LDR ======
uint16_t pupilSizeLeft = 50;// (IRIS_MIN + IRIS_MAX) / 2;
uint16_t pupilSizeRight = 50; //(IRIS_MIN + IRIS_MAX) / 2;
//uint32_t lastPupilUpdate = 0;
//const uint32_t PUPIL_UPDATE_INTERVAL = 100; // Actualizar cada segundo

// ====== Variables para derrame ocular ======
uint32_t lastHemorrhageUpdate = 0;
const uint32_t HEMORRHAGE_UPDATE_INTERVAL = 5000;
bool hemorrhageActive = true;
uint8_t hemorrhageIntensity = 80;
uint8_t hemorrhageSize = 30;
uint8_t hemorrhagePositionX = 64;
uint8_t hemorrhagePositionY = 64;
uint8_t hemorrhageCount = 1;

//uint32_t lastPupilUpdateLeft = 0;
//uint32_t lastPupilUpdateRight = 0;
//const uint32_t PUPIL_UPDATE_LEFT = 200;  // 200ms para ojo izquierdo
//const uint32_t PUPIL_UPDATE_RIGHT = 400; // 400ms para ojo derecho (más lento)

#if !defined(LIGHT_PIN) || (LIGHT_PIN < 0)
// Autonomous iris motion uses a fractal behavior to similate both the major
// reaction of the eye plus the continuous smaller adjustments that occur.
uint16_t oldIris = (IRIS_MIN + IRIS_MAX) / 2, newIris;
#endif
// ====== Función para generar derrame ocular ======
void updateHemorrhage() {
  if (millis() - lastHemorrhageUpdate > HEMORRHAGE_UPDATE_INTERVAL) {
    lastHemorrhageUpdate = millis();
    hemorrhageActive = (random(100) < 70); // 70% de probabilidad
    
    if (hemorrhageActive) {
      hemorrhageIntensity = random(70, 95);
      hemorrhageSize = random(25, 45);
      hemorrhageCount = random(1, 3);
      hemorrhagePositionX = random(30, 98);
      hemorrhagePositionY = random(30, 98);
      
      Serial.print("Hemorrhage active! Size: ");
      Serial.print(hemorrhageSize);
      Serial.print(", Position: (");
      Serial.print(hemorrhagePositionX);
      Serial.print(",");
      Serial.print(hemorrhagePositionY);
      Serial.println(")");
    }
  }
}
// ====== Función para aplicar efecto de hemorragia ======
uint16_t applyHemorrhageEffect(uint16_t x, uint16_t y, uint16_t originalColor) {
  if (!hemorrhageActive) return originalColor;
  
  // Solo aplicar a la esclerótica (parte blanca)
  if (originalColor != TFT_WHITE) {
    return originalColor;
  }
  
  // FORZAR: área cuadrada roja para pruebas
  if (x >= 50 && x <= 78 && y >= 50 && y <= 78) {
    return TFT_RED; // Rojo puro
  }
  
  return originalColor;
}

// Initialise eyes ---------------------------------------------------------
void initEyes(void)
{
  Serial.println("Initialise eye objects");
  // Initialise eye objects based on eyeInfo list in config.h:
  for (uint8_t e = 0; e < NUM_EYES; e++) {
    Serial.print("Create display #"); Serial.println(e);
    eye[e].tft_cs      = eyeInfo[e].select;
    eye[e].blink.state = NOBLINK;
    eye[e].xposition   = eyeInfo[e].xposition;
    pinMode(eye[e].tft_cs, OUTPUT);
    digitalWrite(eye[e].tft_cs, LOW);
    // Also set up an individual eye-wink pin if defined:
    if (eyeInfo[e].wink >= 0) pinMode(eyeInfo[e].wink, INPUT_PULLUP);
  }

#if defined(BLINK_PIN) && (BLINK_PIN >= 0)
  pinMode(BLINK_PIN, INPUT_PULLUP); // Ditto for all-eyes blink pin
#endif
}
//------------------------------------------
// UPDATE EYE --------------------------------------------------------------
// MANTIEN variables globales pero asegura que se actualice
void updatePupilSizeFromLDR() {
  // Actualiza las variables globales
  pupilSizeLeft = map(analogRead(LDRojoizquierdo), 300, 2000, IRIS_MIN, IRIS_MAX );
  pupilSizeRight = map(analogRead(LDRojoderecho), 300, 2000, IRIS_MIN, IRIS_MAX);  
  pupilSizeLeft = (pupilSizeLeft > 140)? 100 : pupilSizeLeft;
  pupilSizeRight = (pupilSizeRight > 140)? 100 : pupilSizeRight;
  pupilSizeLeft = (pupilSizeLeft < 100)? 70 : pupilSizeLeft;
  pupilSizeRight = (pupilSizeRight < 100 )? 70 : pupilSizeRight;
  
  Serial.print("LDR Left: "); Serial.println(pupilSizeLeft);
  Serial.print("LDR Right: "); Serial.println(pupilSizeRight);
}
void updateEye(uint8_t e) {
  // ¡LLAMAR SIEMPRE primero!
  updatePupilSizeFromLDR();
  
  // Usar los valores actualizados
  if (e == 0) {
    frame(e, pupilSizeLeft);
  } else {
    frame(e, pupilSizeRight);
  }
}
// EYE-RENDERING FUNCTION --------------------------------------------------
void drawEye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT) {
  uint32_t screenX, screenY, scleraXsave;
  int32_t irisX, irisY;
  uint32_t p, a;
  uint32_t d;
  uint32_t pixels = 0;

  digitalWrite(eye[e].tft_cs, LOW);
  tft.setRotation(eyeInfo[e].rotation);
  tft.startWrite();
  tft.setAddrWindow((240 - 128) / 2, (240 - 128) / 2, 128, 128);

  scleraXsave = scleraX;
  irisY = scleraY - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;

  uint16_t lidX = 0;
  uint16_t dlidX = -1;
  if (e) dlidX = 1;

  for (screenY = 0; screenY < SCREEN_HEIGHT; screenY++, scleraY++, irisY++) {
    scleraX = scleraXsave;
    irisX = scleraXsave - (SCLERA_WIDTH - IRIS_WIDTH) / 2;
    if (e) lidX = 0; else lidX = SCREEN_WIDTH - 1;
    
    for (screenX = 0; screenX < SCREEN_WIDTH; screenX++, scleraX++, irisX++, lidX += dlidX) {
      // --- LÓGICA ORIGINAL PARA DETERMINAR EL COLOR ---
      if ((pgm_read_byte(lower + screenY * SCREEN_WIDTH + lidX) <= lT) ||
          (pgm_read_byte(upper + screenY * SCREEN_WIDTH + lidX) <= uT)) {
        p = COLOR_PARPA;
      } else if ((irisY < 0) || (irisY >= IRIS_HEIGHT) ||
                 (irisX < 0) || (irisX >= IRIS_WIDTH)) {
        p = pgm_read_word(sclera + scleraY * SCLERA_WIDTH + scleraX);
      } else {
        p = pgm_read_word(polar + irisY * IRIS_WIDTH + irisX);
        d = (iScale * (p & 0x7F)) / 128;
        if (d < IRIS_MAP_HEIGHT) {
          a = (IRIS_MAP_WIDTH * (p >> 7)) / 512;
          p = pgm_read_word(iris + d * IRIS_MAP_WIDTH + a);
          
          uint8_t r = (p >> 11) << 3;
          uint8_t g = ((p >> 5) & 0x3F) << 2;
          uint8_t b = (p & 0x1F) << 3;
          uint8_t intensity = (r * 77 + g * 150 + b * 29) >> 8;
          
          uint16_t newColor = COLOR_IRIS;
          uint8_t newR = ((newColor >> 11) & 0x1F) * intensity / 255;
          uint8_t newG = ((newColor >> 5) & 0x3F) * intensity / 255;
          uint8_t newB = (newColor & 0x1F) * intensity / 255;
          
          p = (newR << 11) | (newG << 5) | newB;
        } else {
          p = pgm_read_word(sclera + scleraY * SCLERA_WIDTH + scleraX);
        }
      }
      
      // --- APLICAR HEMORRAGIA AQUÍ MISMO ---
      p = applyHemorrhageEffect(screenX, screenY, p);
      
      // --- CONTINUAR CON EL CÓDIGO ORIGINAL ---
      uint16_t swappedColor = (p >> 8) | (p << 8);
      *(&pbuffer[dmaBuf][0] + pixels++) = swappedColor;

      if (pixels >= BUFFER_SIZE) {
        yield();
        #ifdef USE_DMA
        tft.pushPixelsDMA(&pbuffer[dmaBuf][0], pixels);
        dmaBuf = !dmaBuf;
        #else
        tft.pushPixels(pbuffer, pixels);
        #endif
        pixels = 0;
      }
    }
  }

  if (pixels) {
    #ifdef USE_DMA
    tft.pushPixelsDMA(&pbuffer[dmaBuf][0], pixels);
    #else
    tft.pushPixels(pbuffer, pixels);
    #endif
  }

  tft.endWrite();
  digitalWrite(eye[e].tft_cs, HIGH);
  delayMicroseconds(10);
}

// EYE ANIMATION -----------------------------------------------------------

const uint8_t ease[] = { // Ease in/out curve for eye movements 3*t^2-2*t^3
  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  3,   // T
  3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  9,  9, 10, 10,   // h
  11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23,   // x
  24, 25, 26, 27, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 38, 39,   // 2
  40, 41, 42, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 56, 57, 58,   // A
  60, 61, 62, 63, 65, 66, 67, 69, 70, 72, 73, 74, 76, 77, 78, 80,   // l
  81, 83, 84, 85, 87, 88, 90, 91, 93, 94, 96, 97, 98, 100, 101, 103, // e
  104, 106, 107, 109, 110, 112, 113, 115, 116, 118, 119, 121, 122, 124, 125, 127, // c
  128, 130, 131, 133, 134, 136, 137, 139, 140, 142, 143, 145, 146, 148, 149, 151, // J
  152, 154, 155, 157, 158, 159, 161, 162, 164, 165, 167, 168, 170, 171, 172, 174, // a
  175, 177, 178, 179, 181, 182, 183, 185, 186, 188, 189, 190, 192, 193, 194, 195, // c
  197, 198, 199, 201, 202, 203, 204, 205, 207, 208, 209, 210, 211, 213, 214, 215, // o
  216, 217, 218, 219, 220, 221, 222, 224, 225, 226, 227, 228, 228, 229, 230, 231, // b
  232, 233, 234, 235, 236, 237, 237, 238, 239, 240, 240, 241, 242, 243, 243, 244, // s
  245, 245, 246, 246, 247, 248, 248, 249, 249, 250, 250, 251, 251, 251, 252, 252, // o
  252, 253, 253, 253, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255
}; // n

#ifdef AUTOBLINK
uint32_t timeOfLastBlink = 0L, timeToNextBlink = 0L;
#endif

// Process motion for a single frame of left or right eye
void frame(uint8_t eyeIndex, uint16_t iScale) // Iris scale (0-1023)
{
  static uint32_t frames   = 0; // Used in frame rate calculation
  int16_t         eyeX, eyeY;
  uint32_t        t = micros(); // Time at start of function
  static uint8_t uThreshold = 180; // Valor fijo para párpado superior (más alto = más abierto)
  static uint8_t lThreshold = 10;  // CAMBIADO: De 100 a 40 (más bajo = párpado inferior más abajo)
  lThreshold = 254 - uThreshold;
  int n =lThreshold;
  if (!(++frames & 255)) { // Every 256 frames...
    float elapsed = (millis() - startTime) / 1000.0;
    if (elapsed) Serial.println((uint16_t)(frames / elapsed)); // Print FPS
  }
  // X/Y movement

  #if defined(JOYSTICK_X_PIN) && (JOYSTICK_X_PIN >= 0) && \
    defined(JOYSTICK_Y_PIN) && (JOYSTICK_Y_PIN >= 0)

    // Read X/Y from joystick, constrain to circle
    int16_t dx, dy;
    int32_t d;
    eyeX = analogRead(JOYSTICK_X_PIN); // Raw (unclipped) X/Y reading
    eyeY = analogRead(JOYSTICK_Y_PIN);
    #ifdef JOYSTICK_X_FLIP
     eyeX = 1023 - eyeX;
    #endif
    #ifdef JOYSTICK_Y_FLIP
      eyeY = 1023 - eyeY;
    #endif
    dx = (eyeX * 2) - 1023; // A/D exact center is at 511.5.  Scale coords
    dy = (eyeY * 2) - 1023; // X2 so range is -1023 to +1023 w/center at 0.
    if ((d = (dx * dx + dy * dy)) > (1023 * 1023)) { // Outside circle
      d    = (int32_t)sqrt((float)d);               // Distance from center
      eyeX = ((dx * 1023 / d) + 1023) / 2;          // Clip to circle edge,
      eyeY = ((dy * 1023 / d) + 1023) / 2;          // scale back to 0-1023
    }  
   #else // Autonomous X/Y eye motion
    // Periodically initiates motion to a new random point, random speed,
    // holds there for random period until next motion.
    static bool  eyeInMotion      = false;
    static int16_t  eyeOldX = 512, eyeOldY = 512, eyeNewX = 512, eyeNewY = 512;
    static uint32_t eyeMoveStartTime = 0L;
    static int32_t  eyeMoveDuration  = 0L;  
    int32_t dt = t - eyeMoveStartTime;      // uS elapsed since last eye event
    if (eyeInMotion) {                      // Currently moving?
      if (dt >= eyeMoveDuration) {          // Time up?  Destination reached.
        eyeInMotion      = false;           // Stop moving
        eyeMoveDuration  = random(6000000); // 0-3 sec stop
        eyeMoveStartTime = t;               // Save initial time of stop
        eyeX = eyeOldX = eyeNewX;           // Save position
        eyeY = eyeOldY = eyeNewY;
      } else { // Move time's not yet fully elapsed -- interpolate position
        int16_t e = ease[255 * dt / eyeMoveDuration] + 1;   // Ease curve
        eyeX = eyeOldX + (((eyeNewX - eyeOldX) * e) / 256); // Interp X
        eyeY = eyeOldY + (((eyeNewY - eyeOldY) * e) / 256); // and Y
      }
    } else {                                // Eye stopped
      eyeX = eyeOldX;
      eyeY = eyeOldY;
      if (dt > eyeMoveDuration) {           // Time up?  Begin new move.
        int16_t  dx, dy;
        uint32_t d;
        do {                                // Pick new dest in circle
          eyeNewX = 512; //random(1024);
          eyeNewY = 512; //random(1024);
          dx      = (eyeNewX * 2) - 1023;
          dy      = (eyeNewY * 2) - 1023;
        } while ((d = (dx * dx + dy * dy)) > (1023 * 1023)); // Keep trying
        eyeMoveDuration  = random(100000, 144000); // ~1/14 - ~1/7 sec
        eyeMoveStartTime = t;               // Save initial time of move
        eyeInMotion      = true;            // Start move on next frame
      }
    }
  #endif // JOYSTICK_X_PIN etc.

  // Blinking
  #ifdef AUTOBLINK
  // Similar to the autonomous eye movement above -- blink start times
  // and durations are random (within ranges).
    if ((t - timeOfLastBlink) >= timeToNextBlink) { // Start new blink?
      timeOfLastBlink = t;
      uint32_t blinkDuration = random(70000, 72000); // ~1/28 - ~1/14 sec
      // Set up durations for both eyes (if not already winking)
      for (uint8_t e = 0; e < NUM_EYES; e++) {
        if (eye[e].blink.state == NOBLINK) {
          eye[e].blink.state     = ENBLINK;
          eye[e].blink.startTime = t;
          eye[e].blink.duration  = blinkDuration;
        }
      }
      timeToNextBlink = blinkDuration * 3 + random(9000000);
      //mayor random, mas tiempor en parpadear
    }
  #endif
  
    if (eye[eyeIndex].blink.state) { // Eye currently blinking?
      // Check if current blink state time has elapsed
      if ((t - eye[eyeIndex].blink.startTime) >= eye[eyeIndex].blink.duration) {
        // Yes -- increment blink state, unless...
        if ((eye[eyeIndex].blink.state == ENBLINK) && ( // Enblinking and...
  #if defined(BLINK_PIN) && (BLINK_PIN >= 0)
              (digitalRead(BLINK_PIN) == LOW) ||           // blink or wink held...
  #endif
              ((eyeInfo[eyeIndex].wink >= 0) &&
               digitalRead(eyeInfo[eyeIndex].wink) == LOW) )) {
          // Don't advance state yet -- eye is held closed instead
        } else { // No buttons, or other state...
          if (++eye[eyeIndex].blink.state > DEBLINK) { // Deblinking finished?
            eye[eyeIndex].blink.state = NOBLINK;      // No longer blinking
          } else { // Advancing from ENBLINK to DEBLINK mode
            eye[eyeIndex].blink.duration *= 2; // DEBLINK is 1/2 ENBLINK speed
            eye[eyeIndex].blink.startTime = t;
          }
        }
      }
    } else { // Not currently blinking...check buttons!
  #if defined(BLINK_PIN) && (BLINK_PIN >= 0)
      if (digitalRead(BLINK_PIN) == LOW) {
        // Manually-initiated blinks have random durations like auto-blink
        uint32_t blinkDuration = random(36000, 72000);
        for (uint8_t e = 0; e < NUM_EYES; e++) {
          if (eye[e].blink.state == NOBLINK) {
            eye[e].blink.state     = ENBLINK;
            eye[e].blink.startTime = t;
            eye[e].blink.duration  = blinkDuration;
          }
        }
      } else
  #endif
        if ((eyeInfo[eyeIndex].wink >= 0) &&
            (digitalRead(eyeInfo[eyeIndex].wink) == LOW)) { // Wink!
          eye[eyeIndex].blink.state     = ENBLINK;
          eye[eyeIndex].blink.startTime = t;
          eye[eyeIndex].blink.duration  = random(45000, 90000);
        }
    }
    // Process motion, blinking and iris scale into renderable values  
    // Scale eye X/Y positions (0-1023) to pixel units used by drawEye()
    eyeX = map(eyeX, 0, 1023, 0, SCLERA_WIDTH  - 128);
    eyeY = map(eyeY, 0, 1023, 0, SCLERA_HEIGHT - 128);
  
    // Horizontal position is offset so that eyes are very slightly crossed
    // to appear fixated (converged) at a conversational distance.  Number
    // here was extracted from my posterior and not mathematically based.
    // I suppose one could get all clever with a range sensor, but for now...
    if (NUM_EYES > 1) {
      if (eyeIndex == 1) eyeX += 4;
      else eyeX -= 4;
    }
    if (eyeX > (SCLERA_WIDTH - 128)) eyeX = (SCLERA_WIDTH - 128);

  /*------- descomentar para el seguimiento del parpado al ojo */
    #ifdef TRACKING
      int16_t sampleX = SCLERA_WIDTH  / 2 - (eyeX / 2), // Reduce X influence
              sampleY = SCLERA_HEIGHT / 2 - (eyeY + IRIS_HEIGHT / 4);
      // Eyelid is slightly asymmetrical, so two readings are taken, averaged
      if (sampleY < 0) n = 0;
      else            n = (pgm_read_byte(upper + sampleY * SCREEN_WIDTH + sampleX) +
                             pgm_read_byte(upper + sampleY * SCREEN_WIDTH + (SCREEN_WIDTH - 1 - sampleX))) / 2;
      uThreshold = (uThreshold * 3 + n) / 4; // Filter/soften motion
      // Lower eyelid doesn't track the same way, but seems to be pulled upward
      // by tension from the upper lid.
      lThreshold = 254 - uThreshold;
    #else // No tracking -- eyelids full open unless blink modifies them
      uThreshold = 60; // Cambiado de 0 a 60 (más alto = más abierto)
      lThreshold = 60; // Cambiado de 0 a 60
    #endif
  // The upper/lower thresholds are then scaled relative to the current
  // blink position so that blinks work together with pupil tracking.
  if (eye[eyeIndex].blink.state) { // Eye currently blinking?
    uint32_t s = (t - eye[eyeIndex].blink.startTime);
    if (s >= eye[eyeIndex].blink.duration) s = 255;  // At or past blink end
    else s = 255 * s / eye[eyeIndex].blink.duration; // Mid-blink
    s          = (eye[eyeIndex].blink.state == DEBLINK) ? 1 + s : 256 - s;
    n          = (uThreshold * s + 254 * (257 - s)) / 256;
    lThreshold = (lThreshold * s + 254 * (257 - s)) / 256;
  } else {
    n          = uThreshold;
  }

  // Pass all the derived values to the eye-rendering function:
  drawEye(eyeIndex, iScale, eyeX, eyeY, n, lThreshold);
  if (eyeIndex == (NUM_EYES - 1)) {
    //user_loop(); // Call user code after rendering last eye
  }
}

// AUTONOMOUS IRIS SCALING (if no photocell or dial) -----------------------
/*
#if !defined(LIGHT_PIN) || (LIGHT_PIN < 0)

// Autonomous iris motion uses a fractal behavior to similate both the major
// reaction of the eye plus the continuous smaller adjustments that occur.

void split( // Subdivides motion path into two sub-paths w/randimization
  int16_t  startValue, // Iris scale value (IRIS_MIN to IRIS_MAX) at start
  int16_t  endValue,   // Iris scale value at end
  uint32_t startTime,  // micros() at start
  int32_t  duration,   // Start-to-end time, in microseconds
  int16_t  range) {    // Allowable scale value variance when subdividing

  if (range >= 8) {    // Limit subdvision count, because recursion
    range    /= 2;     // Split range & time in half for subdivision,
    duration /= 2;     // then pick random center point within range:
    int16_t  midValue = (startValue + endValue - range) / 2 + random(range);
    uint32_t midTime  = startTime + duration;
    split(startValue, midValue, startTime, duration, range); // First half
    split(midValue  , endValue, midTime  , duration, range); // Second half
  } else {             // No more subdivisons, do iris motion...
    int32_t dt;        // Time (micros) since start of motion
    int16_t v;         // Interim value
    while ((dt = (micros() - startTime)) < duration) {
      v = startValue + (((endValue - startValue) * dt) / duration);
      if (v < IRIS_MIN)      v = IRIS_MIN; // Clip just in case
      else if (v > IRIS_MAX) v = IRIS_MAX;
      for(uint8_t e=0; e<NUM_EYES; e++) { // Actualizar ambos ojos
        frame(e, v);
      }
    }
  }
}
#endif // !LIGHT_PIN
*/
