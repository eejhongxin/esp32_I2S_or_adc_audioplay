#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#define SAMPLE_RATE (44100)

class set_I2S_adc
{
public:
  bool InitAdcInput(adc1_channel_t adcChannel);

  size_t Read(char* data, int numData);

  size_t Write(char* data, int numData);

  void End();
};