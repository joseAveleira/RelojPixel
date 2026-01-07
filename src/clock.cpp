#include <Arduino.h>
#include <time.h>
#include <string.h>
#include "clock.h"
#include "display.h"
#include "weather.h"

uint16_t *lastClockFrame = nullptr;
bool clockFrameReady = false;

static unsigned long lastClockUpdate = 0;
static unsigned long lastTypewriterUpdate = 0;
static int typewriterPos = 0;
static const char *marqueeMsg = "Reunion 20:00";

// Variables para notificaciones
static String notificationText = "";
static uint16_t notificationColor = 0xFFFF;  // Blanco por defecto
static bool notificationScroll = false;
static int scrollOffset = 0;
static unsigned long lastScrollUpdate = 0;

// Color personalizado del reloj
static uint8_t clockR = 255, clockG = 200, clockB = 0;  // Amarillo por defecto
static GradientType gradientType = GRADIENT_RAINBOW;  // Arcoíris por defecto

void resetTypewriter(bool complete) {
  if (complete) {
    typewriterPos = strlen(marqueeMsg);  // Mostrar todo el mensaje
  } else {
    typewriterPos = 0;  // Reiniciar animación
  }
  lastTypewriterUpdate = millis();
}

void setClockColor(uint8_t r, uint8_t g, uint8_t b, GradientType gradient) {
  clockR = r;
  clockG = g;
  clockB = b;
  gradientType = gradient;
  const char* names[] = {"SOLID", "LIGHT", "RAINBOW"};
  Serial.printf("[CLOCK] Color: RGB(%d, %d, %d) Gradiente: %s\n", r, g, b, names[gradient]);
}

void setNotification(const char *text, const char *colorHex, bool scroll) {
  notificationText = String(text);

  // Convertir color hex (#FFFFFF) a RGB565
  uint32_t hexColor = strtol(colorHex + 1, NULL, 16);  // Saltar el '#'
  uint8_t r = (hexColor >> 16) & 0xFF;
  uint8_t g = (hexColor >> 8) & 0xFF;
  uint8_t b = hexColor & 0xFF;

  // Si el color es negro, cambiarlo a azul (#00B4FF)
  if (r == 0 && g == 0 && b == 0) {
    r = 0;
    g = 180;
    b = 255;
  }

  notificationColor = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

  // Calcular ancho disponible:
  // Desde columna 9 (izquierda) hasta columna 37 desde el final
  // Total ancho: PANEL_RES_X * PANEL_CHAIN = 128px
  // Límite izquierdo: píxel 9
  // Límite derecho: píxel 128 - 37 = 91
  int leftLimit = 9;
  int rightLimit = PANEL_RES_X * PANEL_CHAIN - 37;  // 128 - 37 = 91
  int availableWidth = rightLimit - leftLimit;
  int textWidth = notificationText.length() * 6;  // 6px por carácter con textSize=1

  // Si el texto es más largo que el espacio disponible, forzar scroll
  if (textWidth > availableWidth) {
    notificationScroll = true;
    scrollOffset = availableWidth;  // Empezar desde la derecha del área disponible
  } else {
    notificationScroll = scroll;
    scrollOffset = 0;
  }

  lastScrollUpdate = millis();
  typewriterPos = 0;  // Resetear animación typewriter

  Serial.printf("[NOTIFY] Texto: %s, Color: %s, Scroll: %d, LeftLimit: %d, RightLimit: %d\n",
                text, colorHex, notificationScroll, leftLimit, rightLimit);
}

// Convierte HSV a RGB565 (h: 0-360, s: 0-1, v: 0-1)
static uint16_t hsvTo565Full(int h, float s, float v) {
  while (h < 0) h += 360;
  h = h % 360;
  float c = v * s;
  float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  float m = v - c;
  float r, g, b;
  if (h < 60) { r = c; g = x; b = 0; }
  else if (h < 120) { r = x; g = c; b = 0; }
  else if (h < 180) { r = 0; g = c; b = x; }
  else if (h < 240) { r = 0; g = x; b = c; }
  else if (h < 300) { r = x; g = 0; b = c; }
  else { r = c; g = 0; b = x; }
  uint8_t R = (uint8_t)((r + m) * 255);
  uint8_t G = (uint8_t)((g + m) * 255);
  uint8_t B = (uint8_t)((b + m) * 255);
  return ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
}

static uint16_t hsvTo565(int h) {
  return hsvTo565Full(h, 1.0f, 1.0f);
}

// Convierte RGB a HSV (devuelve h: 0-360)
static void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, int &h, float &s, float &v) {
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
  float maxC = fmaxf(rf, fmaxf(gf, bf));
  float minC = fminf(rf, fminf(gf, bf));
  float delta = maxC - minC;
  
  v = maxC;
  s = (maxC > 0) ? (delta / maxC) : 0;
  
  if (delta < 0.001f) {
    h = 0;
  } else if (maxC == rf) {
    h = (int)(60 * fmodf((gf - bf) / delta, 6));
  } else if (maxC == gf) {
    h = (int)(60 * ((bf - rf) / delta + 2));
  } else {
    h = (int)(60 * ((rf - gf) / delta + 4));
  }
  if (h < 0) h += 360;
}

static uint16_t gradientColor(int x, int width) {
  float t = (float)x / (float)(width - 1);
  int h = (int)(80 - 80 * t);
  if (h < 0) h = 0;
  return hsvTo565(h);
}

void setMarqueeMessage(const char *msg) {
  marqueeMsg = msg;
}

void drawClock() {
  if (millis() - lastClockUpdate < 60) return;
  lastClockUpdate = millis();

  struct tm tinfo;
  if (!getLocalTime(&tinfo, 100)) {
    dma_display->fillScreen(0);
    dma_display->setCursor(2, 10);
    dma_display->setTextSize(1);
    dma_display->setTextColor(dma_display->color565(255, 80, 80));
    dma_display->print("Sin hora");
    return;
  }

  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tinfo);

  const int textSize = 3;
  const int charW = 5 * textSize + 1;
  const int charH = 8 * textSize;
  const int len = 8;
  const int totalW = len * charW;
  int originX = (PANEL_RES_X * PANEL_CHAIN - totalW) / 2;
  if (originX < 0) originX = 0;
  int originY = 10;  // Forzar posición fija para dejar espacio al marquee

  dma_display->fillScreen(0);
  if (lastClockFrame) {
    memset(lastClockFrame, 0, PANEL_RES_Y * PANEL_RES_X * PANEL_CHAIN * 2);
  }
  clockFrameReady = false;

  // Solo mostrar icono calendario si hay notificación
  if (notificationText.length() > 0) {
    // Icono calendario 7x7 píxeles
    static const uint8_t calIcon[7] = {
      0b0101010,  //  █ █ █  (ganchos)
      0b1111111,  // ███████ (borde superior)
      0b1000001,  // █     █
      0b1010101,  // █ █ █ █ (días)
      0b1000001,  // █     █
      0b1010101,  // █ █ █ █
      0b1111111,  // ███████ (borde inferior)
    };
    uint16_t iconCol = dma_display->color565(0, 180, 255);  // Azul
    for (int row = 0; row < 7; row++) {
      for (int col = 0; col < 7; col++) {
        if (calIcon[row] & (1 << (6 - col))) {
          dma_display->drawPixel(1 + col, 1 + row, iconCol);
        }
      }
    }
  }

  // Mostrar notificación si existe, si no mostrar mensaje marquee normal
  const int marqueeSize = 1;
  dma_display->setTextSize(marqueeSize);

  if (notificationText.length() > 0) {
    // Mostrar notificación con clipping para no pisar iconos
    dma_display->setTextColor(notificationColor);

    // Límites de la ventana de visualización
    int leftLimit = 9;    // Columna 9 desde la izquierda
    int rightLimit = PANEL_RES_X * PANEL_CHAIN - 37;  // Columna 37 desde el final = píxel 91

    if (notificationScroll) {
      // Modo scroll: mover el texto y dibujar solo la parte visible
      int availableWidth = rightLimit - leftLimit;

      if (millis() - lastScrollUpdate > 50) {
        lastScrollUpdate = millis();
        scrollOffset--;
        int textWidth = notificationText.length() * 6;
        if (scrollOffset < -textWidth) {
          scrollOffset = availableWidth;  // Volver a empezar desde el límite derecho
        }
      }

      // Dibujar solo los caracteres que están dentro de la ventana
      int xPos = leftLimit + scrollOffset;
      for (size_t i = 0; i < notificationText.length(); i++) {
        int charX = xPos + (i * 6);  // Cada carácter ocupa 6 píxeles

        // Solo dibujar si el carácter está al menos parcialmente visible
        if (charX + 6 >= leftLimit && charX < rightLimit) {
          // Dibujar carácter por carácter con clipping manual
          dma_display->setCursor(charX, 2);

          // Verificar si necesitamos clipear este carácter
          if (charX >= leftLimit && charX + 6 <= rightLimit) {
            // Carácter completamente visible, dibujar normal
            dma_display->print(notificationText[i]);
          } else {
            // Carácter parcialmente visible, necesitamos clipping píxel a píxel
            // Por ahora simplemente no dibujamos caracteres parciales para evitar complejidad
            if (charX >= leftLimit) {
              dma_display->print(notificationText[i]);
            }
          }
        }
      }
    } else {
      // Modo estático: dibujar con clipping
      int xPos = 10;
      for (size_t i = 0; i < notificationText.length(); i++) {
        int charX = xPos + (i * 6);

        // Solo dibujar si está dentro de los límites
        if (charX >= leftLimit && charX + 6 <= rightLimit) {
          dma_display->setCursor(charX, 2);
          dma_display->print(notificationText[i]);
        } else if (charX >= rightLimit) {
          break;  // Ya salimos del área visible
        }
      }
    }
  }
  // Si no hay notificación, no mostrar nada

  dma_display->setTextSize(textSize);

  for (int idx = 0; idx < len; idx++) {
    int charX = originX + idx * charW;
    uint16_t col;
    float t = (float)(charX + charW / 2) / (PANEL_RES_X * PANEL_CHAIN);
    
    switch (gradientType) {
      case GRADIENT_RAINBOW:
        // Arcoíris original HSV (amarillo → rojo)
        col = gradientColor(charX + charW / 2, PANEL_RES_X * PANEL_CHAIN);
        break;
      case GRADIENT_LIGHT:
        // Gradiente con rotación de hue (más alegre)
        // Rota 60° del color base a lo largo del texto
        {
          int baseH; float baseS, baseV;
          rgbToHsv(clockR, clockG, clockB, baseH, baseS, baseV);
          int newH = baseH + (int)(60 * t);  // Rota hasta 60° en el espectro
          col = hsvTo565Full(newH, baseS, fmaxf(baseV, 0.9f));
        }
        break;
      case GRADIENT_NONE:
      default:
        // Color sólido
        col = dma_display->color565(clockR, clockG, clockB);
        break;
    }
    dma_display->setTextColor(col);
    dma_display->setCursor(charX, originY);
    dma_display->print(timeStr[idx]);
  }

  // Dibujar clima y temperatura en esquina superior derecha
  drawWeather();

  dma_display->flipDMABuffer();
  clockFrameReady = true;
}

void renderClockToBuffer(const char *timeStr, int originX, int originY, int textSize) {
  const int charW = 5 * textSize + 1;
  const int charH = 8 * textSize;
  const int len = 8;
  
  if (!lastClockFrame) return; // Debe estar asignado por quien llama
  memset(lastClockFrame, 0, PANEL_RES_Y * PANEL_RES_X * PANEL_CHAIN * 2);
  
  // Fuente 5x8 compatible con Adafruit GFX (números 0-9 y ':')
  static const uint8_t font5x8[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
  };
  
  for (int idx = 0; idx < len; idx++) {
    int charX = originX + idx * charW;
    uint16_t col;
    float t = (float)(charX + charW / 2) / (PANEL_RES_X * PANEL_CHAIN);
    
    // Usar el mismo sistema de colores que drawClock()
    switch (gradientType) {
      case GRADIENT_RAINBOW:
        col = gradientColor(charX + charW / 2, PANEL_RES_X * PANEL_CHAIN);
        break;
      case GRADIENT_LIGHT:
        // Gradiente con rotación de hue (más alegre)
        {
          int baseH; float baseS, baseV;
          rgbToHsv(clockR, clockG, clockB, baseH, baseS, baseV);
          int newH = baseH + (int)(60 * t);
          // Convertir HSV a RGB565 directamente
          col = hsvTo565Full(newH, baseS, fmaxf(baseV, 0.9f));
        }
        break;
      case GRADIENT_NONE:
      default:
        col = ((clockR & 0xF8) << 8) | ((clockG & 0xFC) << 3) | (clockB >> 3);
        break;
    }
    
    char c = timeStr[idx];
    int fontIdx = -1;
    if (c >= '0' && c <= '9') fontIdx = c - '0';
    else if (c == ':') fontIdx = 10;
    
    if (fontIdx < 0) continue;
    
    for (int col_i = 0; col_i < 5; col_i++) {
      uint8_t colBits = font5x8[fontIdx][col_i];
      for (int row = 0; row < 8; row++) {  // 8 filas para coincidir con fuente real
        if (colBits & (1 << row)) {
          for (int sy = 0; sy < textSize; sy++) {
            for (int sx = 0; sx < textSize; sx++) {
              int px = charX + col_i * textSize + sx;
              int py = originY + row * textSize + sy;
              if (px >= 0 && px < PANEL_RES_X * PANEL_CHAIN && py >= 0 && py < PANEL_RES_Y) {
                lastClockFrame[py * (PANEL_RES_X * PANEL_CHAIN) + px] = col;
              }
            }
          }
        }
      }
    }
  }
}
