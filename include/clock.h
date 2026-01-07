#pragma once
#include <stdint.h>
#include "display.h"

extern uint16_t *lastClockFrame;
extern bool clockFrameReady;

void drawClock();
void renderClockToBuffer(const char *timeStr, int originX, int originY, int textSize);
void setMarqueeMessage(const char *msg);
void setNotification(const char *text, const char *colorHex, bool scroll);  // Nueva función para notificaciones
void resetTypewriter(bool complete = false);  // Resetea animación del marquee

// Tipos de gradiente
enum GradientType {
  GRADIENT_NONE = 0,    // Color sólido
  GRADIENT_LIGHT = 1,   // Color base hacia más claro
  GRADIENT_RAINBOW = 2  // Arcoíris original (amarillo → rojo)
};

// Color del reloj (RGB 0-255, tipo de gradiente)
void setClockColor(uint8_t r, uint8_t g, uint8_t b, GradientType gradient = GRADIENT_NONE);
