#pragma once
#include <Arduino.h>

// Actualizar datos del clima desde MQTT
void updateWeather(int temp, const char* icon, bool isDay);

// Dibujar icono y temperatura en la esquina superior derecha
void drawWeather();
