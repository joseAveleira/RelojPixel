#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "display.h"
#include "clock.h"
#include "gif_player.h"
#include "transitions.h"
#include "weather.h"
#include <IoTConnect.h>
#include <ArduinoJson.h>

// Configuración IoTConnect
#define AP_NAME   "PixelClock-Setup"
#define APP_NAME  "PixelClock"

// Configuración hora
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
bool timeOk = false;

int x_pos = 0;
String texto = "Hi";

#include "gif_list.h" // Lista gigante de GIFs

#include <vector>

// Variables para control de reproducción aleatoria
int* shuffledIndices = nullptr;
int shuffleIndex = 0;

// Variables para Playlist personalizada
std::vector<String> customPlaylist;
bool useCustomPlaylist = false;
int customPlaylistIndex = 0;
String lastPlaylistHash = ""; // Para detectar si la playlist cambió

void shuffleGifs() {
  if (!shuffledIndices) {
    shuffledIndices = new int[NUM_GIFS];
    for (int i = 0; i < NUM_GIFS; i++) shuffledIndices[i] = i;
  }
  
  // Algoritmo Fisher-Yates para barajar
  for (int i = NUM_GIFS - 1; i > 0; i--) {
    int j = random(i + 1);
    int temp = shuffledIndices[i];
    shuffledIndices[i] = shuffledIndices[j];
    shuffledIndices[j] = temp;
  }
  
  shuffleIndex = 0;
  Serial.println("[MAIN] GIFs re-barajados");
}

float gifSpeed = 0.9f;
int maxGifLoops = 1;
unsigned long gifDuration = 20000;

unsigned long clockStart = 0;
unsigned long clockDuration = 10000;
// currentGif ya no se usa como índice directo, usaremos shuffledIndices[shuffleIndex]
int gifsPerCycle = 1;  
int gifsPlayedInCycle = 0; // Cuántos GIFs ver antes del reloj

int showState = 1; // 0: Texto, 1: GIF, 2: Reloj

// Control de encendido/apagado (siempre arranca encendido)
bool displayOn = true;
bool ignoreSyncPower = true;  // Ignorar el primer /power del sync inicial

// Estados de configuración
enum SetupState {
  SETUP_INIT,
  SETUP_SHOW_CONNECT_WIFI,
  SETUP_SHOW_SCAN_QR,
  SETUP_WAITING_CONFIG,
  SETUP_CONNECTED,
  SETUP_DONE
};
SetupState setupState = SETUP_INIT;
unsigned long setupMsgStart = 0;
int scrollPos = 0;
unsigned long lastScrollUpdate = 0;

// ========== FUNCIONES DE PANTALLA SETUP ==========

// Prototipos
void forceSyncState();

void clearAndShowText(const char* line1, const char* line2 = nullptr, uint16_t color = 0xFFFF) {
  dma_display->fillScreen(0);
  dma_display->setTextSize(1);
  dma_display->setTextColor(color);
  
  if (line2 == nullptr) {
    // Una sola línea centrada
    int textW = strlen(line1) * 6;
    int x = (PANEL_RES_X * PANEL_CHAIN - textW) / 2;
    dma_display->setCursor(x, 12);
    dma_display->print(line1);
  } else {
    // Dos líneas
    int textW1 = strlen(line1) * 6;
    int x1 = (PANEL_RES_X * PANEL_CHAIN - textW1) / 2;
    dma_display->setCursor(x1, 6);
    dma_display->print(line1);
    
    int textW2 = strlen(line2) * 6;
    int x2 = (PANEL_RES_X * PANEL_CHAIN - textW2) / 2;
    dma_display->setCursor(x2, 18);
    dma_display->print(line2);
  }
  dma_display->flipDMABuffer();
}

void showScrollingText(const char* text, uint16_t color = 0xFFFF) {
  if (millis() - lastScrollUpdate < 30) return;
  lastScrollUpdate = millis();
  
  dma_display->fillScreen(0);
  dma_display->setTextSize(1);
  dma_display->setTextColor(color);
  
  int textW = strlen(text) * 6;
  int screenW = PANEL_RES_X * PANEL_CHAIN;
  
  dma_display->setCursor(scrollPos, 12);
  dma_display->print(text);
  
  scrollPos--;
  if (scrollPos < -textW) {
    scrollPos = screenW;
  }
  
  dma_display->flipDMABuffer();
}

void showRegisterMsg() {
  const char* msg = "1. Registra en joseaveleira.es/IoT y obtén el QR";
  uint16_t magenta = dma_display->color565(255, 100, 255);
  showScrollingText(msg, magenta);
}

void showConnectWiFiMsg() {
  const char* msg = "2. Conecta al WiFi: PixelClock-Setup";
  uint16_t yellow = dma_display->color565(255, 255, 0);
  showScrollingText(msg, yellow);
}

void showScanQRMsg() {
  const char* msg = "3. Escanea el QR y configura tu WiFi";
  uint16_t cyan = dma_display->color565(0, 255, 255);
  showScrollingText(msg, cyan);
}

void showConnectedMsg() {
  uint16_t green = dma_display->color565(0, 255, 0);
  clearAndShowText("CONECTADO!", nullptr, green);
}

void showConnectedWithProgress(float progress) {
  dma_display->fillScreen(0);
  dma_display->setTextSize(1);
  
  // Texto "CONECTADO!" en verde
  uint16_t green = dma_display->color565(0, 255, 0);
  dma_display->setTextColor(green);
  const char* txt = "CONECTADO!";
  int textW = strlen(txt) * 6;
  int x = (PANEL_RES_X * PANEL_CHAIN - textW) / 2;
  dma_display->setCursor(x, 8);
  dma_display->print(txt);
  
  // Barra de progreso
  int barWidth = 100;
  int barHeight = 6;
  int barX = (PANEL_RES_X * PANEL_CHAIN - barWidth) / 2;
  int barY = 22;
  
  // Borde de la barra (gris oscuro)
  uint16_t gray = dma_display->color565(60, 60, 60);
  dma_display->drawRect(barX, barY, barWidth, barHeight, gray);
  
  // Relleno de la barra (gradiente verde a cyan)
  int fillWidth = (int)(progress * (barWidth - 2));
  for (int i = 0; i < fillWidth; i++) {
    float t = (float)i / (barWidth - 2);
    uint8_t r = 0;
    uint8_t g = 255 - (uint8_t)(t * 55);  // 255 -> 200
    uint8_t b = (uint8_t)(t * 255);        // 0 -> 255
    uint16_t col = dma_display->color565(r, g, b);
    dma_display->drawFastVLine(barX + 1 + i, barY + 1, barHeight - 2, col);
  }
  
  dma_display->flipDMABuffer();
}

void showWaitingMsg() {
  uint16_t blue = dma_display->color565(100, 150, 255);
  clearAndShowText("Conectando...", nullptr, blue);
}

// ========== CALLBACKS MQTT ==========

void onMqttMessage(const char* topic, const char* payload) {
  // Parsear JSON del payload (Buffer aumentado para playlist)
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.printf("[MQTT] Error parseando JSON: %s\n", error.c_str());
    return;
  }
  
  // Topic /playlist: {"playlist":["...","..."], "duration":30}
  if (strstr(topic, "/playlist") != nullptr) {
    if (doc.containsKey("duration")) {
       clockDuration = doc["duration"].as<unsigned long>() * 1000; // Segundos a ms
    } else {
       clockDuration = 30000; // Por defecto 30s si no se especifica
    }

    if (doc.containsKey("playlist")) {
       JsonArray list = doc["playlist"];

       // Crear hash simple de la playlist para detectar cambios
       String newHash = "";
       for (const char* path : list) {
         if (path && strlen(path) > 0) {
            newHash += String(path) + ";";
         }
       }

       // Solo actualizar si la playlist cambió
       bool playlistChanged = (newHash != lastPlaylistHash);

       if (playlistChanged) {
         customPlaylist.clear();
         for (const char* path : list) {
           if (path && strlen(path) > 0) {
              customPlaylist.push_back(String(path));
           }
         }
       }

       if (customPlaylist.size() > 0) {
         useCustomPlaylist = true;

         // Solo resetear índice si la playlist cambió
         if (playlistChanged) {
           customPlaylistIndex = 0;
           lastPlaylistHash = newHash;
           showState = 1;
           clockStart = millis() - clockDuration - 1000;
           Serial.printf("[MQTT] Nueva playlist recibida (%d items). Duración reloj: %ums\n", customPlaylist.size(), clockDuration);

           if (displayOn) {
              dma_display->fillScreen(0);
              dma_display->flipDMABuffer();
              dma_display->fillScreen(0);
              dma_display->flipDMABuffer();
           }
         } else {
           Serial.printf("[MQTT] Playlist duplicada ignorada (índice actual: %d)\n", customPlaylistIndex);
         }

       } else {
         useCustomPlaylist = false;
         lastPlaylistHash = "";
         Serial.println("[MQTT] Playlist vacía. Volviendo a modo aleatorio.");
       }
    }
  }

  // Topic /color: ...
  
  // Topic /color: {"color":{"r":205,"g":220,"b":57}, "gradient":"rainbow"}
  if (strstr(topic, "/color") != nullptr) {
    uint8_t r = 255, g = 200, b = 0;
    GradientType gradient = GRADIENT_NONE;
    
    if (doc.containsKey("color")) {
      JsonObject color = doc["color"];
      r = color["r"] | 255;
      g = color["g"] | 200;
      b = color["b"] | 0;
    } else if (doc.containsKey("r")) {
      // Formato directo: {"r":255,"g":100,"b":50}
      r = doc["r"] | 255;
      g = doc["g"] | 200;
      b = doc["b"] | 0;
    }
    
    // Parsear tipo de gradiente
    if (doc.containsKey("gradient")) {
      if (doc["gradient"].is<bool>()) {
        // true = LIGHT, false = NONE
        gradient = doc["gradient"].as<bool>() ? GRADIENT_LIGHT : GRADIENT_NONE;
      } else if (doc["gradient"].is<const char*>()) {
        const char* gtype = doc["gradient"];
        if (strcmp(gtype, "rainbow") == 0) gradient = GRADIENT_RAINBOW;
        else if (strcmp(gtype, "light") == 0) gradient = GRADIENT_LIGHT;
        else gradient = GRADIENT_NONE;
      }
    }
    
    setClockColor(r, g, b, gradient);
  }
  
  // Topic /power: {"on": true/false}
  if (strstr(topic, "/power") != nullptr) {
    // En el primer /power del sync, forzar encendido y notificar al servidor
    if (ignoreSyncPower) {
      ignoreSyncPower = false;
      displayOn = true;
      Serial.println("[MQTT] Sync inicial: forzando encendido");
      
      // Publicar estado encendido al servidor
      char powerTopic[128];
      snprintf(powerTopic, sizeof(powerTopic), "%s/devices/%s/power", 
               IoTConnect.getPublicId(), 
               IoTConnect.getClientId());
      IoTConnect.publish(powerTopic, "{\"on\":true}");
      Serial.println("[MQTT] Publicado encendido al dashboard");
      return;
    }
    
    if (doc.containsKey("on")) {
      bool newState = doc["on"].as<bool>();
      
      // Solo actuar si cambia el estado
      if (newState != displayOn) {
        displayOn = newState;
        Serial.printf("[MQTT] Display %s\n", displayOn ? "ENCENDIDO" : "APAGADO");
        
        if (!displayOn) {
          // Apagar: limpiar ambos buffers
          dma_display->fillScreen(0);
          dma_display->flipDMABuffer();
          dma_display->fillScreen(0);
          dma_display->flipDMABuffer();
        }
        
        // Confirmar el estado al servidor
        char powerTopic[128];
        snprintf(powerTopic, sizeof(powerTopic), "%s/devices/%s/power", 
                 IoTConnect.getPublicId(), 
                 IoTConnect.getClientId());
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"on\":%s}", displayOn ? "true" : "false");
        IoTConnect.publish(powerTopic, payload);
        Serial.printf("[MQTT] Confirmado: %s\n", payload);
      }
    }
  }

  // Topic /weather: {"temp":8.4,"humidity":88,"isDay":true,"code":3,"condition":"Overcast","icon":"partly-cloudy",...}
  if (strstr(topic, "/weather") != nullptr) {
    if (doc.containsKey("temp") && doc.containsKey("icon") && doc.containsKey("isDay")) {
      int temp = doc["temp"].as<int>();  // Convertir a entero
      const char* icon = doc["icon"];
      bool isDay = doc["isDay"].as<bool>();

      updateWeather(temp, icon, isDay);
    }
  }

  // Topic /notify: {"text":"Tu mensaje","color":"#FFFFFF","scroll":true}
  if (strstr(topic, "/notify") != nullptr) {
    if (doc.containsKey("text") && doc.containsKey("color")) {
      const char* text = doc["text"];
      const char* color = doc["color"];
      bool scroll = doc.containsKey("scroll") ? doc["scroll"].as<bool>() : false;

      setNotification(text, color, scroll);
    }
  }

  // TODO: Añadir más topics aquí (gif, marquee, etc.)
}

void forceSyncState() {
  // Intentamos enviar directamente, la librería gestionará si hay error
  char syncTopic[128];
  snprintf(syncTopic, sizeof(syncTopic), "%s/devices/%s/sync", 
            IoTConnect.getPublicId(), 
            IoTConnect.getClientId());
  IoTConnect.publish(syncTopic, "ok");
  Serial.printf("[MQTT] Sync forzado enviado (si hay conexión)\n");
}

void onConnectionChange(bool connected) {
  if (connected) {
    Serial.println("[IOT] Conexion establecida!");
    
    // Suscribirse al topic: publicId/devices/clientId/#
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/devices/%s/#", 
             IoTConnect.getPublicId(), 
             IoTConnect.getClientId());
    
    if (IoTConnect.subscribe(topic)) {
      Serial.printf("[MQTT] Suscrito a: %s\n", topic);
      
      // Enviar sync inicial
      forceSyncState();
      
      // El power ON se enviará cuando llegue la respuesta del sync
      // (ver handler de /power con ignoreSyncPower)
      
      // El power ON se enviará cuando llegue la respuesta del sync
      // (ver handler de /power con ignoreSyncPower)
    } else {
      Serial.printf("[MQTT] Error al suscribirse a: %s\n", topic);
    }
  } else {
    Serial.println("[IOT] Conexion perdida");
  }
}

// ========== SETUP LOOP PERSONALIZADO ==========

bool runSetupWithDisplay() {
  static unsigned long stateChangeTime = 0;
  static unsigned long lastMsgSwitch = 0;
  static int currentMsg = 0;
  
  // Procesar IoTConnect
  IoTConnect.loop();
  
  // Si ya está listo, mostramos "CONECTADO!" y luego terminamos
  if (IoTConnect.isReady()) {
    if (setupState != SETUP_CONNECTED && setupState != SETUP_DONE) {
      setupState = SETUP_CONNECTED;
      stateChangeTime = millis();
      showConnectedMsg();
    }
    
    if (setupState == SETUP_CONNECTED) {
      // Mostrar "CONECTADO!" con barra de progreso durante 5 segundos
      unsigned long elapsed = millis() - stateChangeTime;
      float progress = (float)elapsed / 5000.0f;
      if (progress > 1.0f) progress = 1.0f;
      
      showConnectedWithProgress(progress);
      
      if (elapsed > 5000) {
        setupState = SETUP_DONE;
        return true; // Setup completado
      }
      return false; // Aún mostrando mensaje
    }
    
    return true;
  }
  
  // Si está en modo configuración (portal cautivo)
  if (IoTConnect.isConfigMode()) {
    // Alternar mensajes cada 5 segundos
    if (millis() - lastMsgSwitch > 5000) {
      lastMsgSwitch = millis();
      currentMsg = (currentMsg + 1) % 3;
      scrollPos = PANEL_RES_X * PANEL_CHAIN; // Reset scroll
    }
    
    if (currentMsg == 0) {
      showRegisterMsg();
    } else if (currentMsg == 1) {
      showConnectWiFiMsg();
    } else {
      showScanQRMsg();
    }
    
    return false;
  }
  
  // Estado intermedio (conectando)
  showWaitingMsg();
  return false;
}

// ========== SETUP Y LOOP ==========

void setup() {


  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
   
  // Inicializar display primero
  initDisplay();
  dma_display->setPanelBrightness(25); 
  x_pos = dma_display->width();

  // Inicializar aleatoriedad y barajar lista
  randomSeed(analogRead(0));
  shuffleGifs();
  
  // Mostrar mensaje inicial
  uint16_t blue = dma_display->color565(100, 150, 255);
  clearAndShowText("Iniciando...", nullptr, blue);
  delay(500);
  
  // Inicializar SD a velocidad media-baja (10MHz) para estabilidad
  const int SD_CS_PIN = 5;
  if(!SD.begin(SD_CS_PIN, SPI, 10000000)){ // 10MHz: Balanceado
    Serial.println("Error SD inicial");
  } else {
    Serial.println("SD OK (10MHz)");
  }
  
  // Mostrar mensaje genérico mientras se inicializa
  clearAndShowText("Conectando...", nullptr, blue);
  
  // Inicializar IoTConnect (esto puede entrar en modo portal bloqueante)
  scrollPos = PANEL_RES_X * PANEL_CHAIN;
  
  // Registrar callbacks ANTES de begin() para no perder eventos
  IoTConnect.onMessage(onMqttMessage);
  IoTConnect.onConnectionChange(onConnectionChange);
  
  // Llamamos begin pero luego manejamos el loop nosotros
  // para poder mostrar mensajes en pantalla
  IoTConnect.begin(AP_NAME, APP_NAME);
  
  // Esperar a que esté configurado mostrando mensajes
  while (!runSetupWithDisplay()) {
    delay(10);
  }
  
  // La suscripción MQTT ya se hace en onConnectionChange(), no duplicar aquí
  
  // Configurar NTP una vez conectado al WiFi
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  struct tm t;
  if (getLocalTime(&t, 3000)) {
    timeOk = true;
    Serial.println("Hora NTP OK");
  } else {
    Serial.println("No se obtuvo hora NTP");
  }
  
  dma_display->clearScreen();
  Serial.println("Setup completado - Iniciando reloj");
}

// Variables para detectar reconexión
static unsigned long lastPortalMsgSwitch = 0;
static int portalMsgIndex = 0;
static bool wasInPortalMode = false;

// Función inteligente para elegir archivo (RAW vs GIF)
String getSmartPath(String inputPath) {
  String fullPath = inputPath;
  
  // 1. Asegurar prefijo /gifs/
  if (!fullPath.startsWith("/gifs/")) {
    if (fullPath.startsWith("/")) fullPath = "/gifs" + fullPath;
    else fullPath = "/gifs/" + fullPath;
  }

  // 2. Comprobar si existe versión RAW optimizada
  String rawPath = fullPath;
  rawPath.replace(".gif", ".raw");
  
  if (SD.exists(rawPath)) {
    return rawPath;
  }
  
  return fullPath;
}

void loop() {
  // Mantener IoTConnect activo
  IoTConnect.loop();
  
  // Si vuelve al modo portal (pérdida de conexión), mostrar mensajes
  if (IoTConnect.isConfigMode()) {
    if (!wasInPortalMode) {
      // Acaba de entrar en modo portal
      wasInPortalMode = true;
      portalMsgIndex = 0;
      lastPortalMsgSwitch = millis();
      scrollPos = PANEL_RES_X * PANEL_CHAIN;
      Serial.println("[MAIN] Entró en modo portal - mostrando instrucciones");
    }
    
    // Alternar mensajes cada 5 segundos
    if (millis() - lastPortalMsgSwitch > 5000) {
      lastPortalMsgSwitch = millis();
      portalMsgIndex = (portalMsgIndex + 1) % 3;
      scrollPos = PANEL_RES_X * PANEL_CHAIN;
    }
    
    if (portalMsgIndex == 0) {
      showRegisterMsg();
    } else if (portalMsgIndex == 1) {
      showConnectWiFiMsg();
    } else {
      showScanQRMsg();
    }
    
    return; // No ejecutar el resto del loop mientras esté en portal
  }
  
  // Si salió del modo portal, resetear flag
  if (wasInPortalMode && !IoTConnect.isConfigMode()) {
    wasInPortalMode = false;
    // Mostrar conectado brevemente
    showConnectedMsg();
    delay(2000);
    dma_display->clearScreen();
    Serial.println("[MAIN] Reconectado - volviendo a modo normal");
  }
  
  // Si display está apagado, no hacer nada más
  if (!displayOn) {
    delay(100);  // Reducir consumo CPU
    return;
  }
  
  if (showState == 0) {
    dma_display->clearScreen(); 
    dma_display->setTextColor(dma_display->color565(0, 255, 255));
    dma_display->setCursor(x_pos, 8); 
    dma_display->print(texto);
    
    x_pos -= 1; 
    int textWidth = texto.length() * 12;
    
    if (x_pos < -textWidth) {
      x_pos = dma_display->width();
      showState = 1;
      dma_display->clearScreen();
    }
    
    dma_display->flipDMABuffer();
    delay(20);
    
  } else if (showState == 1) {
    String selectedPath;

    if (useCustomPlaylist && !customPlaylist.empty()) {
        selectedPath = customPlaylist[customPlaylistIndex];
    } else {
        selectedPath = String(gifPaths[shuffledIndices[shuffleIndex]]);
    }

    // Usar la ruta inteligente (detectar RAW vs GIF)
    String finalPath = getSmartPath(selectedPath);
    const char* pathToPlay = finalPath.c_str();
    
    bool played = false;

    if (strstr(pathToPlay, ".raw") != nullptr) {
        played = playRaw(pathToPlay, 40, 1); // 40ms = 25fps (velocidad intermedia)
        if (!played) {
          Serial.println("[MAIN] Re-iniciando SD tras fallo...");
          dma_display->fillScreen(0);
          dma_display->flipDMABuffer();
          dma_display->fillScreen(0);
          dma_display->flipDMABuffer();
          delay(100);
          SD.end();
          delay(50);
          SD.begin(5, SPI, 10000000);
        }
    } else {
        played = playGif(pathToPlay, gifSpeed, maxGifLoops, gifDuration);
    }
    
    if (played) {
      forceSyncState(); 
      IoTConnect.loop(); 

      if (!displayOn) {
          dma_display->clearScreen();
          dma_display->flipDMABuffer();
      }

      // Avanzar índice según el modo
      if (useCustomPlaylist && !customPlaylist.empty()) {
          customPlaylistIndex = (customPlaylistIndex + 1) % customPlaylist.size();
      } else {
          shuffleIndex++;
          if (shuffleIndex >= NUM_GIFS) {
            shuffleGifs();
          }
      }

      gifsPlayedInCycle++;

      // ¿Hemos terminado la tanda de GIFs?
      if (gifsPlayedInCycle >= gifsPerCycle) {
        gifsPlayedInCycle = 0; // Reset para la próxima vez
        if (displayOn) {
          playGifToClockTransition();
        } else {
           dma_display->clearScreen();
           dma_display->flipDMABuffer();
           dma_display->clearScreen();
           dma_display->flipDMABuffer();
        }
        resetTypewriter(true);
        showState = 2;
        clockStart = millis();
      } else {
        // Si no, simplemente limpiamos un poco y vamos al siguiente GIF de inmediato
        dma_display->fillScreen(0);
        dma_display->flipDMABuffer();
      }
    }
  } else if (showState == 2) {
    if (displayOn) {
      drawClock();
    } else {
      // Limpiar agresivamente usando la función nativa
      dma_display->clearScreen(); 
      dma_display->flipDMABuffer();
      dma_display->clearScreen(); 
      dma_display->flipDMABuffer(); 
    }
    
    // El tiempo sigue corriendo aunque esté apagado
    if (millis() - clockStart > clockDuration) {
      if (displayOn) playClockTransitionWithGif();
      showState = 1;
    }
  }
}
