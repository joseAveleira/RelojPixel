# ⏰ RelojPixel - Smart LED Clock for ESP32

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue?style=for-the-badge&logo=espressif)
![PlatformIO](https://img.shields.io/badge/PlatformIO-5.0+-orange?style=for-the-badge&logo=platformio)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)
![RAM Usage](https://img.shields.io/badge/RAM-73KB%2F320KB-success?style=for-the-badge)
![Flash](https://img.shields.io/badge/Flash-968KB%2F1310KB-blue?style=for-the-badge)

**Un reloj LED inteligente con reproducción de GIFs, notificaciones y control remoto por MQTT**

📝 **[Lee el artículo completo en el blog](https://joseaveleira.es/)** con el proceso de construcción, uso detallado y ejemplos prácticos

[✨ Características](#-características) •
[🛠️ Hardware](#️-hardware-necesario) •
[📦 Instalación](#-instalación) •
[🎮 Uso](#-control-mqtt) •
[🔧 Personalización](#-personalización)

</div>

> 💖 **Agradecimientos Especiales**: Este proyecto tiene sus raíces en [dmdos.net](https://www.dmdos.net/). Un agradecimiento enorme también a [mortaca.com](https://www.mortaca.com/) por la placa que facilita increíblemente todo el montaje. Todo este proyecto salió de allí, ¡gracias!

---

## 🎯 ¿Qué es RelojPixel?

RelojPixel es un reloj LED de **128x32 píxeles** que combina funcionalidad de reloj con un reproductor de GIFs animados y un sistema de notificaciones inteligente. Todo controlado remotamente vía MQTT.

> 📖 **Artículo completo disponible en [joseaveleira.es](https://joseaveleira.es/)** - Incluye guía de construcción paso a paso, configuración detallada y casos de uso reales.

### ✨ Características

#### 🎨 Visuales
- ⏰ **Reloj LED** con 3 tipos de gradientes (sólido, light, arcoíris)
- 🎬 **Reproductor de GIFs** desde tarjeta SD con doble modo (RAM/Streaming)
- 📢 **Notificaciones** con scroll automático y clipping inteligente
- 🌤️ **Widget de clima** con iconos animados (☀️🌧️🌙❄️☁️🌫️)
- ✨ **Transiciones con partículas** (800 partículas con física en tiempo real)

#### 🔌 Conectividad
- 📱 **Portal cautivo WiFi** con configuración automática
- 🔐 **MQTT seguro** con autenticación por tokens
- 📡 **IoTConnect** integrado (o usa tu propio broker)
- 📲 **Configuración QR** para setup en 5 segundos

#### ⚡ Rendimiento
- 🚀 GIFs pequeños (<45KB) cargan en **RAM** para reproducción ultra-rápida
- 🎞️ Formato **RAW** para GIFs pesados: 25fps constantes sin stuttering
- 💾 Solo **73KB de RAM** usados (22.5% del total)
- 📦 Buffer MQTT de **2048 bytes** para playlists largas

---

## 🛠️ Hardware Necesario

| Componente | Especificación |
|------------|----------------|
| **Microcontrolador** | ESP32 DevKit v1 (o compatible) |
| **Paneles LED** | 2x HUB75 RGB (64x32 cada uno) = 128x32 total |
| **Almacenamiento** | MicroSD (mínimo 1GB, cualquier clase) |
| **Alimentación** | 5V DC, 4A mínimo (recomendado 5A) |

### 📐 Esquema de Conexión

```
ESP32          HUB75 Panel
-----          -----------
GPIO 25   ->   R1
GPIO 26   ->   G1
GPIO 27   ->   B1
GPIO 14   ->   R2
GPIO 12   ->   G2
GPIO 13   ->   B2
GPIO 23   ->   A
GPIO 19   ->   B
GPIO 5    ->   C
GPIO 17   ->   D
GPIO 16   ->   E (si tu panel lo usa)
GPIO 4    ->   CLK
GPIO 15   ->   LAT
GPIO 32   ->   OE

MicroSD (SPI)
GPIO 18   ->   SCK
GPIO 19   ->   MISO
GPIO 23   ->   MOSI
GPIO 5    ->   CS
```

---

## 📦 Instalación

### Opción A: Firmware Pre-compilado (Recomendado)

> 💾 **Descarga los binarios listos desde:** [joseaveleira.es](https://joseaveleira.es/)

1. **Descarga los archivos necesarios:**
   - `bootloader.bin`
   - `partitions.bin`
   - `firmware.bin`
   - `manifest.json`

2. **Flashea usando ESP Web Tools:**
   - Ve a [joseaveleira.es](https://joseaveleira.es/) y usa el instalador web
   - Conecta el ESP32 por USB
   - Sigue las instrucciones en pantalla

3. **O usa esptool manualmente:**
   ```bash
   esptool.py --port COM3 --baud 460800 write_flash \
     0x1000 bootloader.bin \
     0x8000 partitions.bin \
     0x10000 firmware.bin
   ```

### Opción B: Compilar desde Código Fuente

```bash
# 1. Clonar repositorio
git clone https://github.com/tuusuario/RelojPixel.git
cd RelojPixel

# 2. Instalar dependencias (PlatformIO CLI)
pio pkg install

# 3. Compilar y subir
pio run --target upload

# 4. Ver monitor serial
pio device monitor
```

---

## 📱 Configuración Inicial

### 1️⃣ Primera Conexión

Al encender el ESP32 por primera vez:

1. Crea una red WiFi: **`PixelClock-Setup`**
2. Conéctate desde tu móvil (sin contraseña)
3. **Se abre automáticamente** una página de configuración (portal cautivo)

Si no se abre, ve manualmente a: `http://192.168.4.1`

> 📝 **Guía detallada con capturas:** [joseaveleira.es](https://joseaveleira.es/)

### 2️⃣ Configurar WiFi y MQTT

Rellena estos datos en el portal:

```
📡 WiFi
├─ SSID: [Tu red WiFi]
└─ Password: [Tu contraseña WiFi]

🔐 MQTT
├─ Client ID: relojpixel-abc123
├─ Public ID: u_12345abc
└─ Token: tu_token_secreto_aqui
```

### 3️⃣ Obtener Credenciales MQTT

#### 🌐 Opción A: Usando joseaveleira.es (Recomendado)

1. Ve a **[joseaveleira.es/IoT](https://joseaveleira.es/IoT)**
2. Regístrate y crea un dispositivo
3. **Escanea el código QR** desde el portal cautivo
4. ¡Listo! Se configura automáticamente

#### 🏠 Opción B: Broker MQTT Local/Propio

Si prefieres usar tu propio broker MQTT (ej: Mosquitto):

1. **Modifica el servidor MQTT** en `.pio/libdeps/esp32dev/IoTConnect/src/Config.h`:
   ```cpp
   constexpr const char* MQTT_HOST = "tu-servidor.com"; // o "192.168.1.100"
   constexpr uint16_t    MQTT_PORT = 1883;
   ```

2. **Recompila y flashea:**
   ```bash
   pio run --target upload
   ```

3. **En el portal cautivo**, usa:
   - **Client ID**: Cualquier nombre único (ej: `mi_reloj_led`)
   - **Public ID**: Tu usuario MQTT
   - **Token**: Tu contraseña MQTT

---

## 🎮 Control MQTT

### 📡 Estructura de Topics

Todos los topics siguen este patrón:
```
{publicId}/devices/{clientId}/{comando}
```

**Ejemplo real:**
```
u_12345abc/devices/relojpixel-abc123/power
u_12345abc/devices/relojpixel-abc123/notify
```

### 📋 Topics Disponibles

#### 1. Encender/Apagar

**Topic:** `{publicId}/devices/{clientId}/power`

```json
{"on": true}   // Encender
{"on": false}  // Apagar
```

#### 2. Cambiar Color del Reloj

**Topic:** `{publicId}/devices/{clientId}/color`

```json
{
  "color": {"r": 255, "g": 200, "b": 0},
  "gradient": "rainbow"
}
```

**Gradientes disponibles:**
- `"solid"` - Color sólido
- `"light"` - Gradiente suave del color base
- `"rainbow"` - Arcoíris amarillo → rojo

#### 3. Playlist de GIFs

**Topic:** `{publicId}/devices/{clientId}/playlist`

```json
{
  "playlist": [
    "Consoles/NES_Super_Mario_Bros_01.gif",
    "Consoles/SNES_MarioKart04.gif",
    "Halloween/HALLOWEEN_SNES_SuperMarioDiv12.gif"
  ],
  "duration": 60
}
```

- `playlist`: Array de rutas de GIFs (relativas a `/gifs/` en la SD)
- `duration`: Segundos que se muestra el **reloj** entre cada GIF

#### 4. Notificaciones

**Topic:** `{publicId}/devices/{clientId}/notify`

```json
{
  "text": "Reunion a las 15:00",
  "color": "#00B4FF",
  "scroll": true
}
```

- `text`: Texto a mostrar (deja vacío `""` para quitar notificación)
- `color`: Color en formato hexadecimal (negro se convierte a azul automáticamente)
- `scroll`: `true` para scroll automático, `false` para texto estático

**Características especiales:**
- 📏 **Clipping automático**: El texto nunca pisa los iconos (calendario/clima)
- ♾️ **Auto-scroll**: Si el texto es muy largo, scroll se activa automáticamente
- 📅 **Icono de calendario**: Solo aparece cuando hay notificación activa

#### 5. Clima

**Topic:** `{publicId}/devices/{clientId}/weather`

```json
{
  "temp": 22,
  "icon": "cloudy-day",
  "isDay": true
}
```

**Iconos disponibles:**
- `"sunny"` / `"clear-day"` → ☀️ Sol
- `"cloudy-day"` / `"cloudy-night"` → ☁️ Nubes
- `"rainy"` → 🌧️ Lluvia
- `"snowy"` → ❄️ Nieve
- `"foggy"` → 🌫️ Niebla
- `"clear-night"` → 🌙 Noche despejada

### 🧪 Ejemplos con Mosquitto

```bash
# Enviar notificación
mosquitto_pub -h joseaveleira.es -t "u_12345/devices/mireloj/notify" \
  -m '{"text":"Hola Mundo","color":"#FF0000","scroll":true}'

# Cambiar color a verde con gradiente arcoíris
mosquitto_pub -h joseaveleira.es -t "u_12345/devices/mireloj/color" \
  -m '{"color":{"r":0,"g":255,"b":0},"gradient":"rainbow"}'

# Actualizar clima
mosquitto_pub -h joseaveleira.es -t "u_12345/devices/mireloj/weather" \
  -m '{"temp":18,"icon":"rainy","isDay":true}'

# Cambiar playlist
mosquitto_pub -h joseaveleira.es -t "u_12345/devices/mireloj/playlist" \
  -m '{"playlist":["gif1.gif","gif2.gif"],"duration":30}'
```

---

## 🎨 Reproducción de GIFs

### 📂 Estructura de la Tarjeta SD

```
SD Card (FAT32)
└── gifs/
    ├── Consoles/
    │   ├── NES_Super_Mario_Bros_01.gif
    │   └── SNES_MarioKart04.gif
    ├── Halloween/
    │   └── HALLOWEEN_Ghost.gif
    └── BEST_OF_TOP_30/
        └── FavoriteAnimation.gif
```

### 🚀 Dos Modos de Reproducción

#### Modo RAM (GIFs < 45KB)
```
✅ Ultra rápido
✅ Sin stuttering
✅ Carga completa en memoria
⚠️ Solo para GIFs pequeños
```

#### Modo Streaming (GIFs > 45KB)
```
✅ Sin límite de tamaño
✅ Lee frame a frame desde SD
⚠️ Un poco más lento
```

### 🎞️ Formato RAW (Avanzado)

Para GIFs muy pesados que necesitan máximo rendimiento, puedes convertirlos a formato RAW:

1. **Convierte GIF a RAW** (pre-procesado):
   ```
   GIF (comprimido) → RAW (píxeles directos en RGB565)
   ```

2. **Ventajas del formato RAW:**
   - ⚡ 25fps perfectos y constantes
   - 🚫 Sin decodificación (píxeles ya listos)
   - 📈 Velocidad predecible sin variaciones

3. **Ubicación:**
   - Los archivos `.raw` se colocan en la misma carpeta que los `.gif`
   - El sistema detecta automáticamente y usa el `.raw` si existe

### 🎨 Colección de GIFs

> 📦 **Descarga packs de GIFs listos para usar:** [joseaveleira.es](https://joseaveleira.es/)
> Incluye: Retro Games, Pixel Art, Animaciones, y más categorías

---

## 🔬 Aspectos Técnicos Destacados

> 🤖 **Desarrollado con asistencia de Claude Sonnet 4.5** - La IA ayudó a optimizar la gestión de RAM, implementar física de partículas en tiempo real, y resolver desafíos técnicos complejos que habrían requerido semanas de desarrollo manual.

### 💾 Gestión Extrema de RAM

El ESP32 tiene **solo 320KB de RAM**, y RelojPixel usa únicamente **73KB (22.5%)**:

```
16KB  → Framebuffer DMA (doble buffer para paneles LED)
45KB  → Buffer GIFs en RAM (modo rápido)
12KB  → Stack y variables del sistema
```

### 🎯 Sistema de Clipping Manual

La librería Adafruit GFX no tiene clipping nativo. Solución implementada:

```cpp
// Renderizado carácter por carácter con límites precisos
for (char c : notificationText) {
  if (charX >= leftLimit && charX + 6 <= rightLimit) {
    display->print(c);  // Solo si está dentro de límites
  }
}
```

**Resultado:** Texto perfectamente acotado entre iconos sin overlap.

### 📦 Buffer MQTT Aumentado

```cpp
// De 1024 bytes → 2048 bytes
mqttClient.setBufferSize(2048);
```

**Permite:** Playlists de hasta ~30-40 GIFs en un solo mensaje JSON.

### 🔄 Hash de Playlist

Evita reseteos innecesarios cuando llegan mensajes MQTT duplicados:

```cpp
if (hashPlaylist(nueva) == hashPlaylist(anterior)) {
  // Ignorar mensaje duplicado
  return;
}
```

### 🌡️ Iconos de Clima Hardcoded

En lugar de cargar imágenes desde SD:

```cpp
static const uint8_t sunIcon[8][8] = {
  {0,0,0,1,1,0,0,0},
  {0,1,1,1,1,1,1,0},
  // ... solo 64 bytes por icono
};
```

**6 iconos = 384 bytes totales** (vs varios KB si fueran archivos externos).

---

## 🔧 Personalización

### 🎨 Cambiar Colores Predeterminados

**Archivo:** `src/clock.cpp`

```cpp
// Color del reloj al arrancar (línea 24)
static uint8_t clockR = 255, clockG = 200, clockB = 0;  // Amarillo

// Gradiente por defecto (línea 25)
static GradientType gradientType = GRADIENT_RAINBOW;

// Color del icono de calendario (línea 139)
uint16_t iconCol = dma_display->color565(0, 180, 255);  // Azul #00B4FF
```

### 📏 Ajustar Límites de Notificaciones

**Archivo:** `src/clock.cpp` (líneas 61-62)

```cpp
int leftLimit = 9;                              // Píxel desde la izquierda
int rightLimit = PANEL_RES_X * PANEL_CHAIN - 37;  // Píxel desde la derecha
```

### 🔌 Cambiar Servidor MQTT

**Archivo:** `.pio/libdeps/esp32dev/IoTConnect/src/Config.h`

```cpp
constexpr const char* MQTT_HOST = "tu-broker.com";
constexpr uint16_t    MQTT_PORT = 1883;
```

Luego recompila:
```bash
pio run --target upload
```

### ⚡ Ajustar Velocidad de Scroll

**Archivo:** `src/clock.cpp` (línea 165)

```cpp
if (millis() - lastScrollUpdate > 50) {  // 50ms = velocidad scroll
  scrollOffset--;
}
```

### 🎞️ Cambiar FPS de Archivos RAW

**Archivo:** `src/gif_player.cpp` (línea 615)

```cpp
delay(40);  // 40ms = 25fps, usa 33ms para 30fps
```

---

## 📚 Librerías Utilizadas

| Librería | Versión | Función |
|----------|---------|---------|
| **[ESP32 HUB75 LED Matrix](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)** | 3.0.14 | Control de paneles LED con DMA |
| **[Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)** | 1.12.4 | Renderizado de gráficos y texto |
| **[AnimatedGIF](https://github.com/bitbank2/AnimatedGIF)** | 2.2.0 | Decodificación de GIFs animados |
| **[IoTConnect](https://github.com/joseAveleira/IoTConnect)** | 1.0.0 | Portal cautivo + MQTT integrado |
| **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)** | 6.21.5 | Parsing de mensajes JSON |
| **SD** | 2.0.0 | Lectura de archivos desde microSD |

### 🔐 IoTConnect - Portal Cautivo Inteligente

**Características destacadas:**

- 📱 **Detección automática**: Los móviles abren el portal sin hacer nada
- 📲 **Configuración QR**: Escanea y configura en 5 segundos
- 💾 **Almacenamiento NVS**: Credenciales guardadas en flash, no se pierden
- 🔄 **Reconexión automática**: Si pierde WiFi, reconecta solo
- 🔒 **Seguro**: Tokens únicos por dispositivo, sin contraseñas hardcoded

**Flujo de funcionamiento:**
```
1. ESP32 arranca sin config → Crea AP "PixelClock-Setup"
2. Usuario se conecta → Portal cautivo se abre automáticamente
3. Usuario escanea QR o rellena form → Credenciales guardadas en NVS
4. ESP32 se conecta a WiFi + MQTT → ¡Funcionando!
5. Próximos arranques → Conecta directamente (config ya guardada)
```

---

## 📁 Estructura del Proyecto

```
RelojPixel/
├── src/
│   ├── main.cpp           # Loop principal, handlers MQTT, playlist
│   ├── clock.cpp          # Renderizado del reloj y notificaciones
│   ├── gif_player.cpp     # Reproducción GIF/RAW (RAM y streaming)
│   ├── weather.cpp        # Widget de clima con iconos
│   ├── transitions.cpp    # Transiciones con partículas
│   └── gif_list.h         # Lista de GIFs disponibles
│
├── include/
│   ├── clock.h            # Prototipos del reloj
│   ├── weather.h          # Prototipos del clima
│   ├── display.h          # Configuración de paneles LED
│   └── ...
│
├── .pio/libdeps/          # Dependencias (auto-gestionadas)
│   └── esp32dev/
│       └── IoTConnect/    # Librería del portal cautivo
│
├── platformio.ini         # Configuración del proyecto
├── manifest.json          # Para flasheo web (ESP Web Tools)
├── bootloader.bin         # Bootloader ESP32 (0x1000)
├── partitions.bin         # Tabla de particiones (0x8000)
└── firmware.bin           # Aplicación compilada (0x10000)
```

---

## 🐛 Solución de Problemas

### ❌ El ESP32 no crea la red WiFi

**Posibles causas:**
- ⏳ Espera 30 segundos tras encenderlo
- 💾 Verifica que la tarjeta SD esté insertada
- 🔄 Resetea el ESP32 (botón RST)

**Solución:**
```bash
# Borrar configuración NVS y empezar desde cero
pio run --target erase
pio run --target upload
```

### ❌ Notificaciones no se ven

**Posibles causas:**
- ⚫ Color negro (se convierte a azul automáticamente, pero verifica)
- 🎬 Está reproduciendo un GIF (notificaciones solo se ven en modo reloj)

**Solución:**
```bash
# Fuerza texto vacío primero, luego envía notificación
mosquitto_pub -t "tu/topic/notify" -m '{"text":"","color":"#FFF","scroll":false}'
mosquitto_pub -t "tu/topic/notify" -m '{"text":"Hola","color":"#00B4FF","scroll":true}'
```

### ❌ Mensajes MQTT se cortan

**Síntoma:** Playlists largas no funcionan

**Solución:**
Ya está aumentado a 2048 bytes, pero si necesitas más:

```cpp
// .pio/libdeps/esp32dev/IoTConnect/src/MqttClient.cpp (línea 43)
mqttClient.setBufferSize(4096);  // Aumentar a 4KB
```

Recompila:
```bash
pio run --target upload
```

### ❌ GIFs se ven lentos/entrecortados

**Solución 1:** Convierte a formato RAW para mejor rendimiento

**Solución 2:** Reduce el tamaño del GIF (< 45KB para modo RAM)

**Solución 3:** Verifica la velocidad de la SD (mínimo Clase 4)

### ❌ Error al flashear: `invalid header: 0xffffffff`

**Causa:** Flasheaste solo `firmware.bin` en dirección incorrecta

**Solución:** Necesitas flashear los 3 archivos:
```bash
esptool.py --port COM3 write_flash \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin
```

O usa el `manifest.json` con ESP Web Tools.

---

## 🎯 Casos de Uso

### 🏠 Reloj de Casa Inteligente
- Muestra la hora con gradientes personalizables
- Notificaciones de recordatorios (Google Calendar, IFTTT)
- Clima actualizado cada hora (API OpenWeather)

### 🎮 Arcade Retro
- Playlists de GIFs de videojuegos retro
- Transiciones con partículas entre escenas
- Control desde Home Assistant

### 🏢 Oficina/Coworking
- Notificaciones de reuniones compartidas
- Estado de salas (Libre/Ocupado)
- Avisos importantes del equipo

### 🎉 Eventos
- Countdown para eventos
- Mensajes de bienvenida personalizados
- Clima en tiempo real del evento

---

## 📊 Estadísticas del Proyecto

```
Líneas de código:     ~3,500
Tamaño compilado:     968KB (73.9% del flash)
RAM usada:            73KB (22.5%)
Librerías:            7
Archivos fuente:      8
Topics MQTT:          5
Iconos clima:         6 (384 bytes totales)
FPS transiciones:     60fps
Partículas máximas:   800
Buffer MQTT:          2048 bytes
GIF max en RAM:       45KB
Tiempo configuración: 5 segundos (con QR)
```

---

## 🤝 Contribuciones

¡Las contribuciones son bienvenidas! Si quieres mejorar RelojPixel:

1. **Fork** el proyecto
2. Crea una **rama** para tu feature:
   ```bash
   git checkout -b feature/nueva-funcionalidad
   ```
3. **Commit** tus cambios:
   ```bash
   git commit -m "Añade nueva funcionalidad X"
   ```
4. **Push** a tu rama:
   ```bash
   git push origin feature/nueva-funcionalidad
   ```
5. Abre un **Pull Request**

### 💡 Ideas para Contribuir

- 🌍 Soporte para más idiomas (UTF-8)
- 🎵 Integración con Spotify (mostrar canción actual)
- 📸 Modo cámara web (mostrar imagen desde URL)
- 🔔 Más tipos de notificaciones (ej: niveles de prioridad)
- 🎨 Crear más GIFs optimizados para la comunidad

---

## 📄 Licencia

Este proyecto está bajo la licencia **MIT** - mira el archivo [LICENSE](LICENSE) para más detalles.

```
MIT License

Puedes usar, copiar, modificar, fusionar, publicar, distribuir,
sublicenciar y/o vender copias del software libremente.
```

---

## 🙏 Agradecimientos

- **[Claude Sonnet 4.5](https://claude.ai)** por ser un asistente de desarrollo excepcional que ayudó a:
  - Optimizar la gestión de RAM (solo 73KB de 320KB usados)
  - Implementar el sistema de transiciones con partículas (800 partículas en tiempo real)
  - Resolver problemas complejos de clipping y rendering
  - Acelerar enormemente el desarrollo de características avanzadas
- **[ESP32 HUB75 Matrix Panel DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)** por la excelente librería de control de paneles
- **[Adafruit](https://github.com/adafruit)** por la librería GFX
- **[BitBank](https://github.com/bitbank2)** por AnimatedGIF
- **Comunidad ESP32** por todo el soporte y recursos

---

## 📞 Recursos y Soporte

- 📝 **Blog con tutorial completo**: [joseaveleira.es](https://joseaveleira.es/)
- 💾 **Binarios pre-compilados**: [joseaveleira.es](https://joseaveleira.es/)
- 📦 **Packs de GIFs listos**: [joseaveleira.es](https://joseaveleira.es/)
- 🐛 **Reportar bugs**: [GitHub Issues](https://github.com/tuusuario/RelojPixel/issues)
- 💬 **Preguntas**: [GitHub Discussions](https://github.com/tuusuario/RelojPixel/discussions)
- 📧 **Email**: contacto@joseaveleira.es

---

## 🚀 Roadmap

### Versión 1.1 (Próximamente)
- [ ] Soporte UTF-8 para caracteres especiales (ñ, á, é, etc.)
- [ ] Modo "Silent Hours" (apagar automáticamente por la noche)
- [ ] Más efectos de transición
- [ ] API REST además de MQTT
- [ ] Más packs de GIFs temáticos

---

<div align="center">

### ⭐ Si te gusta el proyecto, dale una estrella en GitHub

**Hecho con ❤️ y la ayuda de Claude Sonnet 4.5**

🤖 Desarrollado con asistencia de IA para optimización de RAM y características avanzadas

📝 **[Lee el artículo completo en joseaveleira.es](https://joseaveleira.es/)**

[⬆ Volver arriba](#-relojpixel---smart-led-clock-for-esp32)

</div>
