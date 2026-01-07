#pragma once
#include <Arduino.h>

bool playGif(const char *gifPath, float gifSpeed, int maxGifLoops, unsigned long gifDurationMs);
bool playRaw(const char *rawPath, int frameDelayMs, int loopCount);
