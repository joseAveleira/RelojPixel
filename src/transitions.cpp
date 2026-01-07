#include <Arduino.h>
#include <math.h>
#include <time.h>
#include "transitions.h"
#include "display.h"
#include "clock.h"

struct Particle {
  float x, y;
  float vx, vy;
  uint16_t color;
  bool active;
};

static const int MAX_PARTICLES = 800; // Restaurado: ya no afecta al GIF por ser memoria dinámica
static Particle *particles = nullptr;
static int numParticles = 0;

static void captureParticles() {
  numParticles = 0;
  if (!clockFrameReady || !lastClockFrame || !particles) return;
  
  float cx = (PANEL_RES_X * PANEL_CHAIN) / 2.0f;
  float cy = PANEL_RES_Y / 2.0f;
  
  for (int y = 0; y < PANEL_RES_Y && numParticles < MAX_PARTICLES; y++) {
    for (int x = 0; x < PANEL_RES_X * PANEL_CHAIN && numParticles < MAX_PARTICLES; x++) {
      uint16_t col = lastClockFrame[y * (PANEL_RES_X * PANEL_CHAIN) + x];
      if (col == 0) continue;
      
      float dx = x - cx;
      float dy = y - cy;
      float dist = sqrtf(dx*dx + dy*dy);
      if (dist < 1) {
        float angle = random(0, 628) / 100.0f;
        dx = cosf(angle);
        dy = sinf(angle);
        dist = 1;
      }
      
      particles[numParticles].x = (float)x;
      particles[numParticles].y = (float)y;
      float speed = 0.5f + random(100) / 150.0f;
      particles[numParticles].vx = (dx / dist) * speed + (random(-50, 51) / 100.0f);
      particles[numParticles].vy = (dy / dist) * speed + (random(-50, 51) / 100.0f);
      particles[numParticles].color = col;
      particles[numParticles].active = true;
      numParticles++;
    }
  }
  Serial.printf("Partículas creadas: %d\n", numParticles);
}

void playClockTransitionWithGif() {
  struct tm tinfo;
  if (!getLocalTime(&tinfo, 100)) return;

  // Reservar memoria bajo demanda
  if (!lastClockFrame) lastClockFrame = (uint16_t*)malloc(PANEL_RES_Y * PANEL_RES_X * PANEL_CHAIN * 2);
  if (!particles) particles = (Particle*)malloc(MAX_PARTICLES * sizeof(Particle));

  if (!lastClockFrame || !particles) {
    Serial.println("Error: No hay RAM para transición");
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
  int originY = 10;  // Forzar posición fija igual que clock.cpp
  
  renderClockToBuffer(timeStr, originX, originY, textSize);
  clockFrameReady = true;
  
  captureParticles();
  if (numParticles == 0) {
    Serial.println("No hay partículas para animar");
    return;
  }
  
  Serial.printf("Animando %d partículas\n", numParticles);
  
  const int frames = 90;
  
  for (int frame = 0; frame < frames; frame++) {
    dma_display->fillScreen(0);
    
    int activeCount = 0;
    for (int i = 0; i < numParticles; i++) {
      if (!particles[i].active) continue;
      
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].vx *= 1.025f;
      particles[i].vy *= 1.025f;
      particles[i].vy += 0.012f;
      
      int ix = (int)particles[i].x;
      int iy = (int)particles[i].y;
      
      if (ix < 0 || ix >= PANEL_RES_X * PANEL_CHAIN || iy < 0 || iy >= PANEL_RES_Y) {
        particles[i].active = false;
        continue;
      }
      
      dma_display->drawPixel(ix, iy, particles[i].color);
      activeCount++;
    }
    
    dma_display->flipDMABuffer();
    delay(16);
    
    if (activeCount == 0 && frame > 20) break;
  }
  
  // Liberar memoria para que el GIF tenga espacio
  if (particles) { free(particles); particles = nullptr; }
  if (lastClockFrame) { free(lastClockFrame); lastClockFrame = nullptr; }
  clockFrameReady = false;

  // Limpiar AMBOS buffers para evitar artefactos
  for (int buf = 0; buf < 2; buf++) {
    dma_display->fillScreen(0);
    dma_display->flipDMABuffer();
  }
}

void playGifToClockTransition() {
  // Limpiar ambos buffers
  dma_display->fillScreen(0);
  dma_display->flipDMABuffer();
  dma_display->fillScreen(0);
  dma_display->flipDMABuffer();
  
  struct tm tinfo;
  if (!getLocalTime(&tinfo, 100)) return;
  
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tinfo);
  
  const int textSize = 3;
  const int charW = 5 * textSize + 1;
  const int len = 8;
  const int totalW = len * charW;
  int originX = (PANEL_RES_X * PANEL_CHAIN - totalW) / 2;
  if (originX < 0) originX = 0;
  int originY = 10;
  
  // Reservar memoria bajo demanda
  if (!lastClockFrame) lastClockFrame = (uint16_t*)malloc(PANEL_RES_Y * PANEL_RES_X * PANEL_CHAIN * 2);
  if (!particles) particles = (Particle*)malloc(MAX_PARTICLES * sizeof(Particle));

  if (!lastClockFrame || !particles) {
    Serial.println("Error: No hay RAM para transición");
    return;
  }

  // Generar buffer del reloj
  memset(lastClockFrame, 0, PANEL_RES_Y * PANEL_RES_X * PANEL_CHAIN * 2);
  renderClockToBuffer(timeStr, originX, originY, textSize);
  clockFrameReady = true;
  
  // Crear partículas desde el buffer
  numParticles = 0;
  float cx = (PANEL_RES_X * PANEL_CHAIN) / 2.0f;
  float cy = PANEL_RES_Y / 2.0f;
  
  for (int y = 0; y < PANEL_RES_Y && numParticles < MAX_PARTICLES; y++) {
    for (int x = 0; x < PANEL_RES_X * PANEL_CHAIN && numParticles < MAX_PARTICLES; x++) {
      uint16_t col = lastClockFrame[y * (PANEL_RES_X * PANEL_CHAIN) + x];
      if (col == 0) continue;
      
      float dx = x - cx;
      float dy = y - cy;
      float dist = sqrtf(dx*dx + dy*dy);
      
      // Para píxeles cerca del centro, usar ángulo aleatorio
      if (dist < 10) {
        float angle = random(0, 628) / 100.0f;  // 0 a 2*PI
        dx = cosf(angle);
        dy = sinf(angle);
        dist = 1.0f;
      }
      
      // Posición inicial fuera de pantalla (todos vienen de lejos)
      float startDist = 90 + random(40);
      
      particles[numParticles].x = cx + (dx / dist) * startDist;  // Inicio
      particles[numParticles].y = cy + (dy / dist) * startDist;
      particles[numParticles].vx = (float)x;  // Destino
      particles[numParticles].vy = (float)y;
      particles[numParticles].color = col;
      particles[numParticles].active = true;
      numParticles++;
    }
  }
  
  if (numParticles == 0) return;
  Serial.printf("Recomponiendo %d partículas\n", numParticles);
  
  // Animación: 60 frames
  const int frames = 60;
  for (int frame = 0; frame < frames; frame++) {
    dma_display->fillScreen(0);
    
    // Progreso 0.0 a 1.0 con easing
    float t = (float)(frame + 1) / (float)frames;
    float ease = t * t * (3.0f - 2.0f * t);  // smoothstep
    
    for (int i = 0; i < numParticles; i++) {
      float curX = particles[i].x + (particles[i].vx - particles[i].x) * ease;
      float curY = particles[i].y + (particles[i].vy - particles[i].y) * ease;
      
      int ix = (int)roundf(curX);
      int iy = (int)roundf(curY);
      
      if (ix >= 0 && ix < PANEL_RES_X * PANEL_CHAIN && iy >= 0 && iy < PANEL_RES_Y) {
        dma_display->drawPixel(ix, iy, particles[i].color);
      }
    }
    dma_display->flipDMABuffer();
    delay(12);
  }
  
  // Frame final: dibujar directamente desde lastClockFrame (más preciso)
  // En AMBOS buffers para evitar parpadeo
  for (int buf = 0; buf < 2; buf++) {
    dma_display->fillScreen(0);
    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X * PANEL_CHAIN; x++) {
        uint16_t col = lastClockFrame[y * (PANEL_RES_X * PANEL_CHAIN) + x];
        if (col != 0) {
          dma_display->drawPixel(x, y, col);
        }
      }
    }
    dma_display->flipDMABuffer();
  }

  // Liberar memoria tras la transición
  if (particles) { free(particles); particles = nullptr; }
  if (lastClockFrame) { free(lastClockFrame); lastClockFrame = nullptr; }
  clockFrameReady = false;
}
