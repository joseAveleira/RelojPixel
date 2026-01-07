#include "weather.h"
#include "display.h"
#include <string.h>

// Variables privadas del módulo
static int weatherTemp = 0;
static char weatherIcon[32] = "";
static bool weatherIsDay = true;
static bool weatherDataReceived = false;

void updateWeather(int temp, const char* icon, bool isDay) {
  weatherTemp = temp;
  strncpy(weatherIcon, icon, sizeof(weatherIcon) - 1);
  weatherIcon[sizeof(weatherIcon) - 1] = '\0';
  weatherIsDay = isDay;
  weatherDataReceived = true;
  Serial.printf("[WEATHER] Actualizado: %d°C, icon=%s, isDay=%d\n", temp, icon, isDay);
}

void drawWeather() {
  if (!weatherDataReceived || weatherIcon[0] == '\0') {
    return;
  }

  // Iconos 8x8 píxeles (del código que enviaste)
  static const uint8_t sunIcon[8] = {
    0b10010010,  // █  █  █  (rayos superiores)
    0b01000100,  //  █   █   (rayos diagonales)
    0b00011000,  //    ██    (círculo superior más redondo)
    0b10111101,  // █ ████ █ (rayo horizontal + círculo)
    0b00011000,  //    ██    (círculo inferior más redondo)
    0b01000100,  //  █   █   (rayos diagonales)
    0b10010010,  // █  █  █  (rayos inferiores)
    0b00000000,
  };

  static const uint8_t rainIcon[8] = {
    0b00111100,  //   ████   (nube)
    0b01111110,  //  ██████  (nube)
    0b11111111,  // ████████ (nube)
    0b01111110,  //  ██████  (nube)
    0b00000000,
    0b01001001,  //  █  █  █ (gotas superior)
    0b01001001,  //  █  █  █ (gotas inferior - 2px vertical)
    0b00000000,
  };

  static const uint8_t nightIcon[8] = {
    0b00111100,  //   ████
    0b01110000,  //  ███
    0b11100000,  // ███
    0b11100000,  // ███
    0b11100000,  // ███
    0b11100000,  // ███
    0b01110000,  //  ███
    0b00111100,  //   ████
  };

  static const uint8_t fogIcon[8] = {
    0b00000000,
    0b11111110,  // ███████
    0b00000000,
    0b01111110,  //  ██████
    0b00000000,
    0b11111110,  // ███████
    0b00000000,
    0b01111100,  //  █████
  };

  static const uint8_t snowIcon[8] = {
    0b00010000,  //    █     (rayo superior)
    0b01010100,  //  █ █ █   (rayos diagonales)
    0b00101000,  //   █ █    (centro hueco)
    0b11010110,  // ██ █ ██  (rayos horizontales)
    0b00101000,  //   █ █    (centro hueco)
    0b01010100,  //  █ █ █   (rayos diagonales)
    0b00010000,  //    █     (rayo inferior)
    0b00000000,
  };

  static const uint8_t cloudyIcon[8] = {
    0b00000000,
    0b00111100,  //   ████   (nube superior)
    0b01111110,  //  ██████  (nube)
    0b11111111,  // ████████ (nube central)
    0b11111111,  // ████████ (nube central)
    0b01111110,  //  ██████  (nube)
    0b00000000,
    0b00000000,
  };

  // Seleccionar icono según weatherIcon
  const uint8_t *icon = sunIcon;
  uint16_t iconColor = dma_display->color565(255, 200, 0);  // Amarillo para sol

  // REGLA ESPECIAL: Si es "sunny" o "clear" o "partly-cloudy" pero es de noche, mostrar luna
  if ((strcmp(weatherIcon, "sunny") == 0 || strcmp(weatherIcon, "clear") == 0 ||
       strcmp(weatherIcon, "partly-cloudy") == 0) && !weatherIsDay) {
    icon = nightIcon;
    iconColor = dma_display->color565(200, 200, 255);  // Blanco azulado para luna
  } else if (strcmp(weatherIcon, "rain") == 0 || strstr(weatherIcon, "rain") != nullptr) {
    icon = rainIcon;
    iconColor = dma_display->color565(100, 150, 255);  // Azul para lluvia
  } else if (strcmp(weatherIcon, "night") == 0) {
    icon = nightIcon;
    iconColor = dma_display->color565(200, 200, 255);  // Blanco azulado para noche
  } else if (strcmp(weatherIcon, "fog") == 0 || strstr(weatherIcon, "fog") != nullptr ||
             strstr(weatherIcon, "mist") != nullptr) {
    icon = fogIcon;
    iconColor = dma_display->color565(180, 180, 180);  // Gris para niebla
  } else if (strcmp(weatherIcon, "snow") == 0 || strstr(weatherIcon, "snow") != nullptr) {
    icon = snowIcon;
    iconColor = dma_display->color565(220, 240, 255);  // Blanco azulado para nieve
  } else if (strcmp(weatherIcon, "cloudy") == 0 || strstr(weatherIcon, "cloud") != nullptr) {
    icon = cloudyIcon;
    iconColor = dma_display->color565(200, 200, 200);  // Gris claro para nublado
  }

  // Posición: esquina superior derecha (igual que tu código)
  int tempWidth = 24;  // Espacio para temperatura
  int iconX = PANEL_RES_X * PANEL_CHAIN - 8 - 2 - tempWidth;
  int iconY = 1;

  // Dibujar icono
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (icon[row] & (1 << (7 - col))) {
        dma_display->drawPixel(iconX + col, iconY + row, iconColor);
      }
    }
  }

  // Dibujar temperatura "8°C" a la derecha del icono
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(255, 255, 255));  // Blanco
  dma_display->setCursor(iconX + 10, 2);  // 10px a la derecha del icono
  dma_display->printf("%d", weatherTemp);
  dma_display->print((char)247);  // Símbolo de grado °
  dma_display->print("C");
}
