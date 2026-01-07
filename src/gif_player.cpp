#include <SPI.h>
#include <SD.h>
#include <esp_heap_caps.h>
#include <AnimatedGIF.h>
#include "display.h"
#include "gif_player.h"
#include <IoTConnect.h>

// Variable externa para control de encendido
extern bool displayOn;

static AnimatedGIF gif;
static File f;

static uint8_t *gifDataBuf = nullptr;
static size_t gifDataSize = 0;
static char lastLoadedPath[128] = "";
static uint8_t *gifFrameBuffer = nullptr;
static bool gifCookedMode = false;

static void *GIFAlloc(uint32_t size) {
  // Intentar alojar en PSRAM si existe, si no, en RAM interna
  void *p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = malloc(size);
  return p;
}

static void GIFFree(void *p) {
  if (p) free(p);
}

static bool loadGifToMemory(const char *path) {
  if (gifDataBuf && strcmp(lastLoadedPath, path) == 0) return true;
  
  if (gifDataBuf) {
    free(gifDataBuf);
    gifDataBuf = nullptr;
    gifDataSize = 0;
    lastLoadedPath[0] = '\0';
  }

  File file = SD.open(path);
  if (!file) {
    Serial.printf("Error: No existe el archivo %s\n", path);
    return false;
  }

  size_t freeHeap = ESP.getFreeHeap();
  gifDataSize = file.size();
  
  // Si el GIF es muy grande o no hay suficiente heap libre, no cargamos en RAM
  if (gifDataSize > 90000 || (freeHeap < (gifDataSize + 45000))) {
    Serial.printf("GIF pesado (%u bytes) o poca RAM (Heap: %u). Se usará SD.\n", (unsigned)gifDataSize, (unsigned)freeHeap);
    file.close();
    return false;
  }

  gifDataBuf = (uint8_t *)malloc(gifDataSize);
  if (!gifDataBuf) {
    Serial.printf("Fallo malloc para %u bytes (aunque parecía caber), usando SD\n", (unsigned)gifDataSize);
    file.close();
    return false;
  }
  size_t readed = file.read(gifDataBuf, gifDataSize);
  file.close();
  if (readed != gifDataSize) {
    Serial.printf("Lectura incompleta: %u de %u\n", (unsigned)readed, (unsigned)gifDataSize);
    free(gifDataBuf);
    gifDataBuf = nullptr;
    return false;
  }
  strncpy(lastLoadedPath, path, sizeof(lastLoadedPath) - 1);
  Serial.printf("GIF cargado en RAM (%u bytes): %s\n", (unsigned)gifDataSize, path);
  return true;
}

static void *GIFOpenFile(const char *fname, int32_t *pSize) {
  f = SD.open(fname);
  if (f) {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
}

static void GIFCloseFile(void *pHandle) {
  File *fh = (File *)pHandle;
  if (fh != NULL) fh->close();
}

static int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead = iLen;
  File *fh = (File *)pFile->fHandle;
  if (!*fh) {
    Serial.println("ERROR: Archivo no válido en GIFReadFile");
    return 0;
  }
  int32_t remaining = fh->size() - fh->position();
  if (remaining < iLen) iBytesRead = remaining;
  if (iBytesRead <= 0) return 0;
  iBytesRead = fh->read(pBuf, iBytesRead);
  return iBytesRead;
}

static int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *fh = (File *)pFile->fHandle;
  if (!*fh) {
    Serial.println("ERROR: Archivo no válido en GIFSeekFile");
    return 0;
  }
  fh->seek(iPosition);
  return fh->position();
}

static void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  y = pDraw->iY + pDraw->y;
  iWidth = pDraw->iWidth;
  if (y < 0 || y >= PANEL_RES_Y) return;

  if (gifCookedMode) {
    uint16_t *line = (uint16_t *)pDraw->pPixels;
    for (x = 0; x < iWidth; x++) {
      int drawX = x + pDraw->iX;
      if (drawX >= 0 && drawX < (PANEL_RES_X * PANEL_CHAIN)) {
        dma_display->drawPixel(drawX, y, line[x]);
      }
    }
    return;
  }

  usPalette = pDraw->pPalette;
  s = pDraw->pPixels;
  
  if (pDraw->ucDisposalMethod == 2) {
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  if (pDraw->ucHasTransparency) {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0;
    while (x < iWidth) {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent) {
          s--;
        } else {
          *d++ = usPalette[c];
          iCount++;
        }
      }
      if (iCount) {
        for (int xOffset = 0; xOffset < iCount; xOffset++) {
          int drawX = x + xOffset + pDraw->iX;
          if (drawX >= 0 && drawX < (PANEL_RES_X * PANEL_CHAIN)) {
            dma_display->drawPixel(drawX, y, usTemp[xOffset]);
          }
        }
        x += iCount;
        iCount = 0;
      }
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--;
      }
      if (iCount) {
        x += iCount;
        iCount = 0;
      }
    }
  } else {
    s = pDraw->pPixels;
    for (x = 0; x < iWidth; x++) {
      int drawX = x + pDraw->iX;
      if (drawX >= 0 && drawX < (PANEL_RES_X * PANEL_CHAIN)) {
        dma_display->drawPixel(drawX, y, usPalette[*s]);
      }
      s++;
    }
  }
}

bool playGif(const char *gifPath, float gifSpeed, int maxGifLoops, unsigned long gifDurationMs) {
  gif.close(); // Asegurar cierre total previo
  gif.begin(LITTLE_ENDIAN_PIXELS);

  bool opened = false;
  if (loadGifToMemory(gifPath)) {
    opened = gif.open(gifDataBuf, gifDataSize, GIFDraw);
    if (opened) Serial.println("Reproduciendo desde RAM");
    else Serial.printf("open RAM falló, error %d\n", gif.getLastError());
  }
  
  if (!opened) {
    opened = gif.open(gifPath, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
    if (opened) Serial.println("Reproduciendo desde SD");
  }

  if (!opened) {
    Serial.printf("Error abriendo GIF: %s. Error: %d\n", gifPath, gif.getLastError());
    return false;
  }

  Serial.printf("GIF Abierto: %s (%dx%d)\n", gifPath, gif.getCanvasWidth(), gif.getCanvasHeight());
  size_t freeHeap = ESP.getFreeHeap();
  size_t maxBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  Serial.printf("RAM: Total libre %u, bloque más grande %u\n", (unsigned)freeHeap, (unsigned)maxBlock);
  Serial.flush();

  // Estrategia de memoria: 
  // Si el archivo es muy grande (>100KB), no usamos Framebuffer ni Turbo para que 
  // el decodificador LZW tenga todo el espacio contiguo que necesite.
  bool forceRaw = (gifDataSize > 120000 && gifDataBuf == nullptr);
  
  int allocResult = -1; 
  if (!forceRaw) {
    allocResult = gif.allocFrameBuf(GIFAlloc);
  }

  if (allocResult == GIF_SUCCESS) {
    gifFrameBuffer = gif.getFrameBuf();
    gifCookedMode = true;
    gif.setDrawType(GIF_DRAW_COOKED);
    Serial.println("Modo: COOKED (Framebuffer OK)");
    
    // Turbo solo para GIFs en RAM que no sean gigantes
    if (gifDataBuf != nullptr && gifDataSize < 60000) {
       gif.allocTurboBuf(GIFAlloc);
    }
  } else {
    Serial.printf("Modo: RAW (%s)\n", forceRaw ? "Forzado por tamaño" : "Fallo memoria");
    gifCookedMode = false;
  }

  int frameCount = 0;
  int loopsDone = 0;
  unsigned long startTime = millis();

  Serial.println("Reproduciendo GIF por 10 segundos...");
  Serial.flush();
  
  while (millis() - startTime < gifDurationMs) {
    // Si se apagó el display, salir inmediatamente
    if (!displayOn) {
      Serial.println("[GIF] Interrumpido por apagado");
      gif.close();
      dma_display->fillScreen(0);
      dma_display->flipDMABuffer();
      dma_display->fillScreen(0);
      dma_display->flipDMABuffer();
      return true;
    }
    
    int frameDelay = 0;
    int result = gif.playFrame(false, &frameDelay);

    if (result == 1) {
      frameCount++;
      dma_display->flipDMABuffer();
      int effectiveDelay = (int)(frameDelay * gifSpeed);
      if (effectiveDelay < 1) effectiveDelay = 1;
      delay(effectiveDelay);
    } else if (result == 0) {
      loopsDone++;
      Serial.printf("Ciclo completado con %d frames (loop %d/%d)\n", frameCount, loopsDone, maxGifLoops);
      if (loopsDone >= maxGifLoops) {
        break;
      }
      gif.reset();
      frameCount = 0;
    } else {
      int err = gif.getLastError();
      Serial.printf("Frame error, código %d\n", err);
      if (err == GIF_BAD_FILE || err == GIF_EARLY_EOF) {
        Serial.println("Reabriendo GIF tras error...");
        gif.reset();
        break;
      }
      delay(10);
    }
    yield();
  }
  
  unsigned long elapsed = millis() - startTime;
  Serial.printf("GIF terminado: %d frames en %lu ms\n", frameCount, elapsed);
  
  // Liberar buffers internos de AnimatedGIF
  gif.freeTurboBuf(GIFFree);
  gif.freeFrameBuf(GIFFree);
  gif.close();
  gifFrameBuffer = nullptr;

  // Liberar el GIF de la RAM para dejar espacio a las transiciones
  if (gifDataBuf) {
    free(gifDataBuf);
    gifDataBuf = nullptr;
    gifDataSize = 0;
    lastLoadedPath[0] = '\0';
    Serial.println("RAM del GIF liberada para transiciones");
  }

  return true;
}

bool playRaw(const char *rawPath, int frameDelayMs, int loopCount) {
  // 1. Comprobar si existe antes de intentar abrir
  if (!SD.exists(rawPath)) {
    Serial.printf("[RAW] Salteado: No existe %s\n", rawPath);
    return true; // Devolvemos true para que el main NO reinicie la SD (no es fallo de hardware)
  }

  File rawFile = SD.open(rawPath);
  if (!rawFile) {
    Serial.printf("Error SD: No se pudo abrir %s (pese a existir)\n", rawPath);
    // Esto SI huele a fallo de hardware
    SD.end();
    SD.begin(5, SPI, 10000000); 
    return false;
  }

  size_t framePixels = PANEL_RES_X * PANEL_CHAIN * PANEL_RES_Y;
  size_t frameSize = framePixels * 2; // 8192 bytes
  
  static uint8_t *frameBuf = nullptr;
  if (!frameBuf) frameBuf = (uint8_t*)malloc(frameSize); 

  if (!frameBuf) {
    Serial.println("Error: Sin RAM para buffer RAW");
    rawFile.close();
    return false;
  }

  Serial.printf("Iniciando RAW: %s (Loops: %d)\n", rawPath, loopCount);
  
  int currentLoop = 0;
  
  while (currentLoop < loopCount) {
    unsigned long frameStart = millis();

    // Si queda menos de un frame, significa FIN DE ARCHIVO -> FIN DE LOOP
    if (rawFile.available() < frameSize) {
      rawFile.seek(0);
      currentLoop++;
      Serial.printf("[RAW] Loop %d completado\n", currentLoop);
      if (currentLoop >= loopCount) break;
    }

    // Leer 1 frame
    if (rawFile.read(frameBuf, frameSize) != frameSize) {
      Serial.println("[RAW] Error lectura SD");
      break;
    }

    // Pintar
    dma_display->drawRGBBitmap(0, 0, (uint16_t*)frameBuf, PANEL_RES_X * PANEL_CHAIN, PANEL_RES_Y);
    dma_display->flipDMABuffer();
    
    // Si la visualización se apaga, salimos
    if (!displayOn) break;

    // Sincronización inteligente de tiempo
    // Calculamos cuánto hemos tardado en leer/pintar y esperamos solo lo necesario
    long timeUsed = millis() - frameStart;
    long sleepTime = frameDelayMs - timeUsed;
    
    if (sleepTime > 0) {
      delay(sleepTime);
    }

    yield();
  }

  rawFile.close();
  return true;
}
